#include <uhd/utils/thread.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <chrono>
#include <time.h>


#include <sigmf.h>
#include <sigmf_core_generated.h>
#include <sigmf_antenna_generated.h>

#define NUM_SAMPS 1e8
typedef std::complex<int16_t> cint_16;

int UHD_SAFE_MAIN(int argc, char *argv[]) {

    // Set up CLI options
    namespace po = boost::program_options;
    po::options_description desc("record_sigmf program options:");
    desc.add_options()
            ("output,o", po::value<std::string>()->default_value(""), "output filename")
            ("freq,f", po::value<double>()->default_value(880), "center frequency (MHz)")
            ("rate,r", po::value<double>()->default_value(40), "sample rate (MHz)")
            ("bw,w", po::value<double>()->default_value(40), "sample bandwidth (MHz)")
            ("gain,g", po::value<int>()->default_value(45), "receive gain (dB)")
            ("samples,n", po::value<int>()->default_value(NUM_SAMPS), "Number of samples to capture")
            ("seconds,s", po::value<float>()->default_value(0), "Number of seconds to capture")
            ("bufsize,b", po::value<int>()->default_value(100000000), "Number of samples per buffer")
            ("device,d", po::value<std::string>()->default_value(""), "USRP Device Args")
            ("subdev,v", po::value<std::string>()->default_value("A:A"), "USRP Subdevice")
            ("antenna,a", po::value<std::string>()->default_value("RX2"), "USRP Antenna Port")
            ("reference,c", po::value<std::string>()->default_value("internal"), "Clock Reference")
            ("datatype,t", po::value<std::string>()->default_value("16sc"), "Data type")
            ("description,e", po::value<std::string>()->default_value(""), "Description")
            ("shortname,N", po::value<std::string>()->default_value("RF"), "Short name in auto filename")
            ("showjson,j", po::bool_switch()->default_value(false), "Only show JSON Example and Exit")
            ("help,h", "produce help message")
            ("manifest,m", po::value<std::string>()->default_value(""), "Manifest File")
            ;

    if(!boost::filesystem::is_directory("./data")){
        boost::filesystem::create_directories("./data");
    }


    po::variables_map args;
    po::store( po::parse_command_line(argc, argv, desc), args );
    po::notify(args);

    // set up datetime
    auto startTime = std::chrono::system_clock::now();
    auto partial_time_microsec = std::chrono::time_point_cast<std::chrono::microseconds>(startTime) -
                                 std::chrono::time_point_cast<std::chrono::seconds>(startTime);
    auto tt = std::chrono::system_clock::to_time_t(startTime);
    std::stringstream datetime_stringstream;
    datetime_stringstream << std::put_time(std::gmtime(&tt), "%Y-%m-%d %H:%M:%S.") << boost::format("%06d")%partial_time_microsec.count();

    // if help called, or required variable note passed
    if(args.count("help") || args.count("freq") < 1){
        std::cout << desc << "\n";
        return 0;
    }

    // set up the primary SigMF Metadata object from CLI options
    sigmf::SigMF<
            sigmf::VariadicDataClass<core::GlobalT, antenna::GlobalT>,
            sigmf::VariadicDataClass<core::CaptureT>,
            sigmf::VariadicDataClass<core::AnnotationT>
        > meta;

    // Set up global fields
    auto &g = meta.global.access<core::GlobalT>();
    g.sample_rate = args["rate"].as<double>() * 1e6;
    g.datatype = args["datatype"].as<std::string>();
    g.description = args["description"].as<std::string>();

    // Set up capture fields
    auto &c = meta.captures.create_new().access<core::CaptureT>();
    c.frequency = args["freq"].as<double>()*1e6;
    c.sample_start = 0;
    c.datetime = datetime_stringstream.str();

    // Set up Antenna fields
    auto &a = meta.global.access<antenna::GlobalT>();
    a.gain = args["gain"].as<int>();
    a.type = args["antenna"].as<std::string>();

    size_t num_samps = size_t(args["samples"].as<int>());

    if(!args["manifest"].as<std::string>().empty()){
        std::ifstream mf(args["manifest"].as<std::string>(), std::ios::in);
        json j;
        mf >> j;
        for(auto& it : j.items()){
            //std::cout << it.key() << std::endl << " Datatype: " << it.value()["datatype"] << " Description: " << it.value()["description"] << std::endl;



            json val = it.value();
            g.datatype = val["datatype"];
            c.frequency = double(val["freq"]) * 1e6;
            g.sample_rate = double(val["rate"]) * 1e6;
            a.gain = int(val["gain"]);
            num_samps = size_t(val["samples"]);


            std::string file_base = boost::str(
                    boost::format("SIGMF_%s_%s_F%0.3f_R%0.3f_T%0.03f") %
                    (args["shortname"].as<std::string>()) %
                    (g.datatype)  %
                    (c.frequency / 1e6) %
                    (g.sample_rate / 1e6) %
                    ( std::chrono::time_point_cast<std::chrono::milliseconds>(startTime).time_since_epoch().count()/1e3 ));

            std::string filename;
            filename += "./data/";
            std::string prefix = args["output"].as<std::string>();
            if (!prefix.empty())
            {
                filename += prefix;
                filename += "_";
            }

            filename += it.key() + "_" + file_base;



            auto fname_meta = filename + ".sigmf-meta";
            auto fname_data = filename + ".sigmf-data";



            // Debug print out JSON
            if(args["showjson"].as<bool>()){
                std::cout << "filename: " << filename << "\n";
                std::cout << json(meta).dump(2) << "\n";

            }

            // Set up the UHD
            uhd::set_thread_priority_safe();
            uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args["device"].as<std::string>());
            usrp->set_clock_source(args["reference"].as<std::string>());
            usrp->set_rx_subdev_spec(args["subdev"].as<std::string>());
            usrp->set_rx_rate(g.sample_rate);
            uhd::tune_request_t tune_request(c.frequency);
            usrp->set_rx_freq(tune_request);
            usrp->set_rx_gain(a.gain);
            usrp->set_rx_bandwidth(args["bw"].as<double>() * 1e6);
            usrp->set_rx_antenna(a.type);

            // provide some feedback ...
            std::cout << boost::format("USRP Configued with Rate: %f MSPS, Freq: %f MHz, Gain: %d dB, Antenna: %s, NSamp: %d\n") %
                         ( usrp->get_rx_rate() / 1e6 ) %
                         ( usrp->get_rx_freq() / 1e6 ) %
                         ( usrp->get_rx_gain() ) %
                         ( usrp->get_rx_antenna() ) %
                         ( num_samps );

            // some default args .... hard wired
            std::string cpu_format = "sc16";
            std::string wire_format = "sc16";
            typedef std::complex<int16_t> samp_type;
            bool enable_size_map = false;
            int channel = 0;

            // set up streamer
            uhd::stream_args_t stream_args(cpu_format,wire_format);
            std::vector<size_t> channel_nums;
            channel_nums.push_back(channel);
            stream_args.channels = channel_nums;
            uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

            //setup streaming
            uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
            stream_cmd.num_samps = num_samps;
            stream_cmd.stream_now = true;
            stream_cmd.time_spec = uhd::time_spec_t();
            rx_stream->issue_stream_cmd(stream_cmd);

            uhd::rx_metadata_t md;

            size_t actually_received = 0;
            char *buff = (char *)malloc(sizeof(samp_type) * num_samps);

            while(actually_received < num_samps ){
                size_t num_rx_samps = rx_stream->recv(&buff[actually_received * sizeof(samp_type)], num_samps - actually_received, md, 3.0, enable_size_map);
                if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE){

                    uhd::stream_cmd_t rx_stop_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
                    rx_stop_cmd.stream_now = false;
                    rx_stream->issue_stream_cmd( rx_stop_cmd );
                    size_t zero_count = 0;
                    do {
                        num_rx_samps = rx_stream->recv(&buff[0], num_samps, md, 3.0, enable_size_map);
                        std::cout << "FLUSHING SAMPLE BUFFER NumSamps: " << num_rx_samps << std::endl;
                        if (num_rx_samps == 0){
                            zero_count++;
                        }

                    } while(!md.end_of_burst && zero_count < 4);

                    std::cout << "RETRYING CAPTURE" << std::endl;
                    actually_received = 0;
                    rx_stream->issue_stream_cmd(stream_cmd);
                    continue;
                }
                else{
                    actually_received += num_rx_samps;
                    std::cout << "NumRX: " << num_rx_samps << " ActRec: " << actually_received << std::endl;
                }
            }

            std::ofstream fd(fname_data.c_str(), std::ios::out | std::ios::binary);
            fd.write(buff, sizeof(samp_type) * num_samps);
            fd.close();

            // write out meta
            std::ofstream metafile;
            metafile.open(fname_meta);
            metafile << json(meta).dump(2) << "\n";
            metafile.close();

            free(buff);
        }
    }
    else{

        std::string file_base = boost::str(
                boost::format("SIGMF_%s_%s_F%0.3f_R%0.3f_T%0.03f") %
                (args["shortname"].as<std::string>()) %
                (g.datatype)  %
                (c.frequency / 1e6) %
                (g.sample_rate / 1e6) %
                ( std::chrono::time_point_cast<std::chrono::milliseconds>(startTime).time_since_epoch().count()/1e3 ));

        std::string filename;
        std::string prefix = args["output"].as<std::string>();
        if (!prefix.empty())
        {
            filename += prefix;
            filename += "_";
        }

        filename += file_base;

        auto fname_meta = filename + ".sigmf-meta";
        auto fname_data = filename + ".sigmf-data";

        // Debug print out JSON
        if(args["showjson"].as<bool>()){
            std::cout << "filename: " << filename << "\n";
            std::cout << json(meta).dump(2) << "\n";
            return 0;
        }

        // Set up the UHD
        uhd::set_thread_priority_safe();
        uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args["device"].as<std::string>());
        usrp->set_clock_source(args["reference"].as<std::string>());
        usrp->set_rx_subdev_spec(args["subdev"].as<std::string>());
        usrp->set_rx_rate(g.sample_rate);
        uhd::tune_request_t tune_request(c.frequency);
        usrp->set_rx_freq(tune_request);
        usrp->set_rx_gain(a.gain);
        usrp->set_rx_bandwidth(args["bw"].as<double>() * 1e6);
        usrp->set_rx_antenna(a.type);

        // provide some feedback ...
        std::cout << boost::format("USRP Configued with Rate: %f MSPS, Freq: %f MHz, Gain: %d dB, Antenna: %s, NSamp: %d\n") %
                     ( usrp->get_rx_rate() / 1e6 ) %
                     ( usrp->get_rx_freq() / 1e6 ) %
                     ( usrp->get_rx_gain() ) %
                     ( usrp->get_rx_antenna() ) %
                     ( num_samps );


        // some default args .... hard wired
        std::string cpu_format = "sc16";
        std::string wire_format = "sc16";
        typedef std::complex<int16_t> samp_type;
        bool enable_size_map = false;
        int channel = 0;

        // set up streamer
        uhd::stream_args_t stream_args(cpu_format,wire_format);
        std::vector<size_t> channel_nums;
        channel_nums.push_back(channel);
        stream_args.channels = channel_nums;
        uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);


        //setup streaming
        uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
        stream_cmd.num_samps = num_samps;
        stream_cmd.stream_now = true;
        stream_cmd.time_spec = uhd::time_spec_t();
        rx_stream->issue_stream_cmd(stream_cmd);

        uhd::rx_metadata_t md;

        size_t actually_received = 0;
        char *buff = (char *)malloc(sizeof(samp_type) * num_samps);

        while(actually_received < num_samps ){
            size_t num_rx_samps = rx_stream->recv(&buff[actually_received * sizeof(samp_type)], num_samps - actually_received, md, 3.0, enable_size_map);
            if (md.error_code & uhd::rx_metadata_t::ERROR_CODE_OVERFLOW){
                uhd::stream_cmd_t rx_stop_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
                rx_stop_cmd.stream_now = true;
                rx_stream->issue_stream_cmd( rx_stop_cmd );

                do {
                    rx_stream->recv(buff, num_samps, md, 3.0, enable_size_map);
                    std::cout << "Clearing " << std::endl;
                } while(!md.end_of_burst) ;

                std::cout << "RETRYING\n";
                actually_received = 0;
                rx_stream->issue_stream_cmd(stream_cmd);
                continue;
            }
            else{
                actually_received += num_rx_samps;
                std::cout << "NumRX: " << num_rx_samps << " ActRec: " << actually_received << std::endl;
            }
        }

        std::ofstream fd(fname_data.c_str(), std::ios::out | std::ios::binary);
        fd.write(buff, sizeof(samp_type) * num_samps);
        fd.close();

        // write out meta
        std::ofstream metafile;
        metafile.open(fname_meta);
        metafile << json(meta).dump(2) << "\n";
        metafile.close();

        free(buff);
    }


    return EXIT_SUCCESS;
}
