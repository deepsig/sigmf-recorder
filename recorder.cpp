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


template<typename samp_type>
void record_usrp_to_file(uint64_t num_samps, const std::string &fname_data, uhd::usrp::multi_usrp::sptr &usrp,
                         const std::string &cpu_format);

int UHD_SAFE_MAIN(int argc, char *argv[]) {
    // Set up CLI options
    namespace po = boost::program_options;
    po::options_description desc("record_sigmf program options:");
    desc.add_options()
            ("output,o", po::value<std::string>()->default_value(""),
             "output filename if no manifest, otherwise added to manifest filename")
            ("freq,f", po::value<double>()->default_value(880), "center frequency [MHz]")
            ("rate,r", po::value<double>()->default_value(40), "sample rate (MHz)")
            ("bw,w", po::value<double>()->default_value(40), "sample bandwidth (MHz)")
            ("gain,g", po::value<double>()->default_value(45), "receive gain (dB)")
            ("samples,n", po::value<uint64_t>()->default_value(100000000), "Number of samples to capture")
            ("seconds,s", po::value<float>()->default_value(0), "Number of seconds to capture")
            ("bufsize,b", po::value<int>()->default_value(100000000), "Number of samples per buffer")
            ("device,d", po::value<std::string>()->default_value(""), "USRP Device Args")
            ("subdev,v", po::value<std::string>()->default_value("A:A"), "USRP Subdevice")
            ("antenna,a", po::value<std::string>()->default_value("RX2"), "USRP Antenna Port")
            ("reference,c", po::value<std::string>()->default_value("internal"), "Clock Reference")
            ("datatype,t", po::value<std::string>()->default_value("ci16_le"), "Data type")
            ("description,e", po::value<std::string>()->default_value(""), "Description")
            ("shortname,N", po::value<std::string>()->default_value("RF"), "Short name in auto filename")
            ("help,h", "produce help message")
            ("manifest,m", po::value<std::string>()->default_value(""), "Manifest File");

    if (!boost::filesystem::is_directory("./data")) {
        boost::filesystem::create_directories("./data");
    }

    po::variables_map args;
    po::store(po::parse_command_line(argc, argv, desc), args);
    po::notify(args);

    // if help called, or required variable note passed
    if (args.count("help") || args.count("freq") < 1) {
        std::cout << desc << "\n";
        return 0;
    }

    uint64_t num_samps = size_t(args["samples"].as<uint64_t>());

    if (!args["manifest"].as<std::string>().empty()) {
        std::ifstream manifest_file(args["manifest"].as<std::string>(), std::ios::in);
        json recording_manifest;
        manifest_file >> recording_manifest;
        uhd::set_thread_priority_safe();
        uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args["device"].as<std::string>());
        usrp->set_clock_source(args["reference"].as<std::string>());
        usrp->set_rx_subdev_spec(args["subdev"].as<std::string>());

        for (auto &recording_parameters : recording_manifest) {
            std::cout << recording_parameters.dump(2) << std::endl;

            num_samps = recording_parameters["samples"].get<uint64_t>();

            char fileformatted_date[32];
            long now = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            strftime(fileformatted_date, 32, "%Y-%m-%d-%H-%M-%S", localtime(&now));
            auto date_for_filename = std::string(fileformatted_date);

            std::stringstream filename;
            filename << "./data/";
            std::string prefix = args["output"].as<std::string>();
            if (!prefix.empty()) {
                filename << prefix;
                filename << "_";
            }

            filename << fileformatted_date;

            std::string cpu_format = args["datatype"].as<std::string>();
            // Set up the USRP
            usrp->set_rx_rate(recording_parameters["rate"].get<double>() * 1e6);
            usrp->set_rx_bandwidth(recording_parameters["rate"].get<double>() * 1e6);
            uhd::tune_request_t tune_request(recording_parameters["freq"].get<double>() * 1e6);
            usrp->set_rx_freq(tune_request);
            usrp->set_rx_gain(recording_parameters["gain"].get<double>());
            usrp->set_rx_antenna("RX2");

            // set up the primary SigMF Metadata object from CLI options
            sigmf::SigMF<
                    sigmf::VariadicDataClass<core::GlobalT, antenna::GlobalT>,
                    sigmf::VariadicDataClass<core::CaptureT>,
                    sigmf::VariadicDataClass<core::AnnotationT>
            > recording_metadata;
            // Set up Antenna fields
            auto &antenna_metadata = recording_metadata.global.access<antenna::GlobalT>();
            antenna_metadata.type = args["antenna"].as<std::string>();

            // Set up global fields
            auto &global_metadata = recording_metadata.global.access<core::GlobalT>();
            global_metadata.datatype = args["datatype"].as<std::string>();
            global_metadata.description = args["description"].as<std::string>();

            // Set up capture fields
            auto &captures_metadata = recording_metadata.captures.create_new().access<core::CaptureT>();
            captures_metadata.sample_start = 0;
            captures_metadata.datetime = date_for_filename;

            global_metadata.datatype = recording_parameters["datatype"];
            captures_metadata.frequency = usrp->get_rx_freq();
            global_metadata.sample_rate = usrp->get_rx_rate();

            filename << "_" << args["output"].as<std::string>() << "_" << std::setw(2)
                     << captures_metadata.frequency / 1e6;
            auto filename_string = filename.str();
            auto fname_meta = filename_string + ".sigmf-meta";
            auto fname_data = filename_string + ".sigmf-data";
            std::cout << "Filename for meta is " << fname_meta << std::endl;
            std::cout << "Filename for data is " << fname_meta << std::endl;

            std::cout << "cpu format is " << cpu_format << std::endl;
            if (cpu_format == "ci8_le") {
                record_usrp_to_file<std::complex<int8_t> >(num_samps, fname_data, usrp, "sc8");
            } else if (cpu_format == "ci16_le") {
                record_usrp_to_file<std::complex<int16_t> >(num_samps, fname_data, usrp, "sc16");
            } else if (cpu_format == "cf32_le") {
                record_usrp_to_file<std::complex<float> >(num_samps, fname_data, usrp, "fc32");
            }

            // write out meta
            std::ofstream metafile;
            metafile.open(fname_meta);
            metafile << json(recording_metadata).dump(2) << "\n";
            metafile.close();
        }
    } else {
        // Fill in call to record when there is no manifest

    }

    return EXIT_SUCCESS;
}

template<typename samp_type>
void record_usrp_to_file(uint64_t num_samps, const std::string &fname_data, uhd::usrp::multi_usrp::sptr &usrp,
                         const std::string &cpu_format) {
    std::string wire_format = "sc16";

    // set up streamer
    int channel = 0;
    uhd::stream_args_t stream_args(cpu_format, wire_format);
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

    char *buff = (char *) malloc(sizeof(samp_type) * num_samps);

    size_t actually_received = 0;
    uhd::rx_metadata_t md;
    while (actually_received < num_samps) {
        size_t num_rx_samps = rx_stream->recv(&buff[actually_received * sizeof(samp_type)],
                                              num_samps - actually_received, md, 3.0);
        if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
            uhd::stream_cmd_t rx_stop_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
            rx_stop_cmd.stream_now = false;
            rx_stream->issue_stream_cmd(rx_stop_cmd);
            size_t zero_count = 0;
            do {
                num_rx_samps = rx_stream->recv(&buff[0], num_samps, md, 0.1);
                std::cout << "FLUSHING SAMPLE BUFFER NumSamps: " << num_rx_samps << std::endl;
                if (num_rx_samps == 0) {
                    zero_count++;
                }

            } while (!md.end_of_burst && zero_count < 4);

            std::cout << "RETRYING CAPTURE" << std::endl;
            actually_received = 0;
            rx_stream->issue_stream_cmd(stream_cmd);
            continue;
        } else {
            actually_received += num_rx_samps;
            std::cout << "NumRX: " << num_rx_samps << " ActRec: " << actually_received << std::endl;
        }
    }

    std::cout << "Sample type is " << typeid(samp_type).name() << std::endl;
    std::ofstream fd(fname_data.c_str(), std::ios::out | std::ios::binary);
    fd.write(buff, sizeof(samp_type) * num_samps);
    fd.close();
    free(buff);
}
