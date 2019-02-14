#include <uhd/utils/thread.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <fstream>
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
            ("freq,f", po::value<double>(), "center frequency (MHz) [required]")
            ("rate,r", po::value<double>()->default_value(40), "sample rate (MHz)")
            ("bw,w", po::value<double>()->default_value(40), "sample bandwidth (MHz)")
            ("gain,g", po::value<int>()->default_value(45), "receive gain (dB)")
            ("samples,n", po::value<int>()->default_value(100000000), "Number of samples to capture")
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
            ;
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

    // parse out the length of file to capture (use samples is specified, otherwise seconds, otherwise 0 = inf)

    size_t nsamp = 100000000;
    float sec = args["seconds"].as<float>();
//    if(nsamp == 0 && g.sample_rate > 0 && sec > 0){
//        nsamp = g.sample_rate * sec;
//    }

    // Check for filename and make one up from metadata and time if not specified
    std::string filebase = args["output"].as<std::string>();
    if(filebase == ""){
        filebase = boost::str(
                boost::format("SIGMF_%s_%s_F%0.3f_R%0.3f_T%0.03f") %
                (args["shortname"].as<std::string>()) %
                (g.datatype)  %
                (c.frequency / 1e6) %
                (g.sample_rate / 1e6) %
                ( std::chrono::time_point_cast<std::chrono::milliseconds>(startTime).time_since_epoch().count()/1e3 )
        );
    }
    auto fname_meta = filebase + ".sigmf-meta";
    auto fname_data = filebase + ".sigmf-data";

    // Debug print out JSON
    if(args["showjson"].as<bool>()){
        std::cout << "filebase: " << filebase << "\n";
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
                 ( nsamp );


    // some default args .... hard wired
    std::string cpu_format = "sc16";
    std::string wire_format = "sc16";
    typedef std::complex<int16_t> samp_type;
//    typedef std::complex<float> samp_type;
    bool enable_size_map = false;
    int channel = 0;

    // set up streamer
    uhd::stream_args_t stream_args(cpu_format,wire_format);
    std::vector<size_t> channel_nums;
    channel_nums.push_back(channel);
    stream_args.channels = channel_nums;
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    start_rec:
    // open output file
    //auto fd = fopen(fname_data.c_str(), "w");

//    //setup streaming
//    uhd::stream_cmd_t stream_cmd((nsamp == 0)?
//                                 uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS:
//                                 uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE
//    );
//    stream_cmd.num_samps = size_t(100000000);
//    stream_cmd.stream_now = true;
//    stream_cmd.time_spec = uhd::time_spec_t();
//    rx_stream->issue_stream_cmd(stream_cmd);
//
//    // very simple receive loop ... to be improved
//    uhd::rx_metadata_t md;
//    //std::vector<samp_type> buff(args["bufsize"].as<int>());
//    std::vector<samp_type> buff(100000000);
//    auto location = buff.begin();
//    size_t nrx(100000000);
//    bool finished = false;
//    unsigned long long actually_received = 0;

    //setup streaming
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    stream_cmd.num_samps = NUM_SAMPS;
    stream_cmd.stream_now = true;
    stream_cmd.time_spec = uhd::time_spec_t();
    rx_stream->issue_stream_cmd(stream_cmd);

    uhd::rx_metadata_t md;

    unsigned long long actually_received = 0;
    char *buff = (char *)malloc(sizeof(cint_16) * NUM_SAMPS);

    while(actually_received < NUM_SAMPS ){
        size_t num_rx_samps = rx_stream->recv(&buff[actually_received * sizeof(cint_16)], NUM_SAMPS - actually_received, md, 3.0, enable_size_map);
        if (md.error_code & uhd::rx_metadata_t::ERROR_CODE_OVERFLOW){
            std::cout << "RETRYING\n";
            actually_received = 0;
            continue;
        }
        else{
            //location += num_rx_samps;
            actually_received += num_rx_samps;
            std::cout << "NumRX: " << num_rx_samps << " ActRec: " << actually_received << std::endl;
        }
//        size_t nsamp_use = (nsamp==0)?num_rx_samps:std::min(nsamp-nrx, num_rx_samps);
//        nrx += nsamp_use;

        //finished = true;
    }

    std::ofstream fd(fname_data.c_str(), std::ios::out | std::ios::binary);
    //fwrite(&buff[0], sizeof(samp_type), 100000000, fd);
    fd.write(buff, sizeof(cint_16) * NUM_SAMPS);
    fd.close();

    // write out meta
    std::ofstream metafile;
    metafile.open(fname_meta);
    metafile << json(meta).dump(2) << "\n";
    metafile.close();

    // ...
    //std::cout << "finished. " << nrx << " samples written: " << fname_meta <<  "\n";
    return EXIT_SUCCESS;
}
