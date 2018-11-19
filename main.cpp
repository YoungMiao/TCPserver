#include "SRSocket.h"
#include "SRHelper.h"
#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "tree/context-dep.h"
#include "hmm/transition-model.h"
#include "fstext/fstext-lib.h"
#include "decoder/decoder-wrappers.h"
#include "nnet3/nnet-am-decodable-simple.h"
#include "nnet3/nnet-utils.h"
#include "base/timer.h"

int main()
{   
    using namespace kaldi;
    using namespace kaldi::nnet3;
    typedef kaldi::int32 int32;
    using fst::SymbolTable;
    using fst::Fst;
    using fst::StdArc;

    const char* usage =
        "Generate lattices using nnet3 neural net model.\n"
        "Usage: nnet3-latgen-faster [options] <nnet-in> <fst-in|fsts-rspecifier> <features-rspecifier>"
        " <lattice-wspecifier> [ <words-wspecifier> [<alignments-wspecifier>] ]\n"
        "See also: nnet3-latgen-faster-parallel, nnet3-latgen-faster-batch\n";
    ParseOptions po(usage);
    Timer timer;
    bool allow_partial = false;
    LatticeFasterDecoderConfig config;
    NnetSimpleComputationOptions decodable_opts;

    std::string word_syms_filename;
    std::string ivector_rspecifier,
        online_ivector_rspecifier,
        utt2spk_rspecifier;
    int32 online_ivector_period = 0;
    config.Register(&po);
    decodable_opts.Register(&po);
    po.Register("word-symbol-table", &word_syms_filename,
                "Symbol table for words [for debug output]");
    po.Register("allow-partial", &allow_partial,
                "If true, produce output even if end state was not reached.");
    po.Register("ivectors", &ivector_rspecifier, "Rspecifier for "
                "iVectors as vectors (i.e. not estimated online); per utterance "
                "by default, or per speaker if you provide the --utt2spk option.");
    po.Register("utt2spk", &utt2spk_rspecifier, "Rspecifier for "
                "utt2spk option used to get ivectors per speaker");
    po.Register("online-ivectors", &online_ivector_rspecifier, "Rspecifier for "
                "iVectors estimated online, as matrices.  If you supply this,"
                " you must set the --online-ivector-period option.");
    po.Register("online-ivector-period", &online_ivector_period, "Number of frames "
                "between iVectors in matrices supplied to the --online-ivectors "
                "option");

    po.Read(argc, argv);

    if (po.NumArgs() < 4 || po.NumArgs() > 6) {
        po.PrintUsage();
        exit(1);
    }

    std::string model_in_filename = po.GetArg(1),
                fst_in_str = po.GetArg(2),
                feature_rspecifier = po.GetArg(3),
                lattice_wspecifier = po.GetArg(4),
                words_wspecifier = po.GetOptArg(5),
                alignment_wspecifier = po.GetOptArg(6);
    KALDI_LOG << "model_in_filename:" << model_in_filename;
    KALDI_LOG << "fst_in_str:" << fst_in_str;
    KALDI_LOG << "feature_rspecifier:" << feature_rspecifier;
    KALDI_LOG << "lattice_wspecifier:" << lattice_wspecifier;

    TransitionModel trans_model;
    AmNnetSimple am_nnet;
    {
        bool binary;
        Input ki(model_in_filename, &binary);
        trans_model.Read(ki.Stream(), binary);
        am_nnet.Read(ki.Stream(), binary);
        SetBatchnormTestMode(true, &(am_nnet.GetNnet()));
        SetDropoutTestMode(true, &(am_nnet.GetNnet()));
        CollapseModel(CollapseModelConfig(), &(am_nnet.GetNnet()));
    }
    gettimeofday(&t2, NULL);
    double deltaT = ((t2.tv_usec - t1.tv_usec) + (t2.tv_sec - t1.tv_sec) * 1000000) / 1000;

    fst::SymbolTable* word_syms = NULL;
    if (word_syms_filename != "")
        if (!(word_syms = fst::SymbolTable::ReadText(word_syms_filename)))
            KALDI_ERR << "Could not read symbol table from file "
                      << word_syms_filename;
    Fst<StdArc>* decode_fst = fst::ReadFstKaldiGeneric(fst_in_str);
    LatticeFasterDecoder decoder(*decode_fst, config);

    bool determinize = config.determinize_lattice;
    CompactLatticeWriter compact_lattice_writer;
    LatticeWriter lattice_writer;

    RandomAccessBaseFloatMatrixReader online_ivector_reader(online_ivector_rspecifier);
    RandomAccessBaseFloatVectorReaderMapped ivector_reader(ivector_rspecifier, utt2spk_rspecifier);

    KALDI_LOG << "ivector_rspecifier:" << ivector_rspecifier;
    KALDI_LOG << "utt2spk_rspecifier:" << utt2spk_rspecifier;

    Int32VectorWriter words_writer(words_wspecifier);
    Int32VectorWriter alignment_writer(alignment_wspecifier);



    std::string result;
    double tot_like = 0.0;
    kaldi::int64 frame_count = 0;
    int num_success = 0, num_fail = 0;
    // this compiler object allows caching of computations across
    // different utterances.
    CachingOptimizingCompiler compiler(am_nnet.GetNnet(), decodable_opts.optimize_config);

    KALDI_LOG << "server start!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n" << "use time:" << deltaT << "ms";
	SRHelper::instance()->a = 1;
	SRSocketSever server(5,8989);
	server.start();
	return 0;
}