
#include <stdio.h>

#include "TBranch.h"
#include "TBufferFile.h"
#include "TFile.h"
#include "TTree.h"
#include "TStopwatch.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include "TTreeReaderFast.h"
#include "TTreeReaderValueFast.h"

int main(int argc, char *argv[]) {

    TFile *hfile;
    TTree *tree;

    // Handle all the argument parsing up front.
    if (argc != 5) {
        fprintf(stderr, "Usage: %s read|write|writelz4|writezip|writelzma|writeuncompressed bulk|bulkinline|fastreader|standard events fname\n", argv[0]);
        return 1;
    }
    bool do_read = false, do_lz4 = false, do_zip = false, do_uncompressed = false, do_lzma = false;
    if (!strcmp(argv[1], "read")) {
        do_read = true;
    } else if (!strcmp(argv[1], "writelz4")) {
        do_lz4 = true;
    } else if (!strcmp(argv[1], "writezip")) {
        do_zip = true;
    } else if (!strcmp(argv[1], "writelzma")) {
        do_lzma = true;
    } else if (!strcmp(argv[1], "writeuncompressed")) {
        do_uncompressed = true;
    } else if (strcmp(argv[1], "write")) {
        fprintf(stderr, "Second argument must be 'read', 'write', 'writelz4', 'writezip', or 'writeuncompressed'\n");
        return 1;
    }
    bool do_fast_reader = false;
    bool do_std = false;
    bool do_inline = false;
    if (!strcmp(argv[2], "standard")) {
        do_std = true;
    } else if (!strcmp(argv[2], "bulkinline")) {
        do_inline = true;
    } else if (!strcmp(argv[2], "fastreader")) {
        do_fast_reader = true;
    } else if (strcmp(argv[2], "bulk")) {
        fprintf(stderr, "Third argument must be either 'bulk', 'fastreader', or 'standard'\n");
    }
    Long64_t events;
    try {
        events = std::stol(argv[3]);
    } catch (...) {
        fprintf(stderr, "Failed to parse third argument (%s) to integer.\n", argv[3]);
        return 1;
    }
    const char *fname = argv[4];
    // End arg parsing.

    hfile = new TFile(fname);
    if (do_read) {
        printf("Starting read of file %s.\n", fname);
            TStopwatch sw;

        if (do_std) {
            printf("Using standard read APIs.\n");
            // Read via standard APIs.
            TTreeReader myReader("T", hfile);
            TTreeReaderValue<float> myF(myReader, "myFloat");
            Long64_t idx = 0;
            float idx_f = 1;
            sw.Start();
            while (myReader.Next()) {
                if (R__unlikely(idx == events)) {break;}
                idx_f++;
                if (R__unlikely((idx < 16000000) && (*myF != idx_f))) {
                    printf("Incorrect value on myFloat branch: %f, expected %f (event %ld)\n", *myF, idx_f, idx);
                    return 1;
                }
                idx++;
            }
        } else if (do_fast_reader) {
            printf("Using faster reader APIs.\n");
            TTreeReaderFast myReader("T", hfile);
            TTreeReaderValueFast<float> myF(myReader, "myFloat");
            myReader.SetEntry(0);
            if (ROOT::Internal::TTreeReaderValueBase::kSetupMatch != myF.GetSetupStatus()) {
               printf("TTreeReaderValueFast<float> failed to initialize.  Status code: %d\n", myF.GetSetupStatus());
               return 1;
            }
            if (myReader.GetEntryStatus() != TTreeReader::kEntryValid) {
               printf("TTreeReaderFast failed to initialize.  Entry status: %d\n", myReader.GetEntryStatus());
               return 1;
            }
            Long64_t idx = 0;
            float idx_f = 1;
            for (auto it : myReader) {
                if (R__unlikely(idx == events)) {break;}
                idx_f++;
                if (R__unlikely((idx < 16000000) && (*myF != idx_f))) {
                    printf("Incorrect value on myFloat branch: %f, expected %f (event %ld)\n", *myF, idx_f, idx);
                    return 1;
                }
                idx++;
            }
        } else if (do_inline) {
            printf("Using inline bulk read APIs.\n");
            TBufferFile branchbuf(TBuffer::kWrite, 32*1024);
            TTree *tree = dynamic_cast<TTree*>(hfile->Get("T"));
            if (!tree) {
                std::cout << "Failed to fetch tree named 'T' from input file.\n";
                return 1;
            }
            TBranch *branchF = tree->GetBranch("myFloat");
            if (!branchF) {
                std::cout << "Unable to find branch 'myFloat' in tree 'T'\n";
                return 1;
            }
            float idx_f = 1;
            Long64_t evt_idx = 0;
            while (events) {
                auto count = branchF->GetEntriesSerialized(evt_idx, branchbuf);
                if (R__unlikely(count < 0)) {
                    printf("Failed to get entries via the 'serialized' method for index %d.\n", evt_idx);
                    return 1;
                }
                if (events > count) {
                    events -= count;
                } else {
                    events = 0;
                }
                float *entry = reinterpret_cast<float*>(branchbuf.GetCurrent());
                for (Int_t idx=0; idx<count; idx++) {
                    idx_f++;
                    Int_t *buf = reinterpret_cast<Int_t*>(&entry[idx]);
                    *buf = __builtin_bswap32(*buf);

                    if (R__unlikely((evt_idx < 16000000) && (entry[idx] != idx_f))) {
                    //if (R__unlikely((evt_idx < 16000000) && (entry[idx] == -idx_f))) {
                        printf("Incorrect value on myFloat branch: %f (event %ld)\n", entry[idx], evt_idx + idx);
                        return 1;
                    }
                }
                evt_idx += count;
            }
        } else {
            printf("Using bulk read APIs.\n");
            // Read using bulk APIs.
            TBufferFile branchbuf(TBuffer::kWrite, 32*1024);
            TTree *tree = dynamic_cast<TTree*>(hfile->Get("T"));
            if (!tree) {
                std::cout << "Failed to fetch tree named 'T' from input file.\n";
                return 1;
            }
            TBranch *branchF = tree->GetBranch("myFloat");
            if (!branchF) {
                std::cout << "Unable to find branch 'myFloat' in tree 'T'\n";
                return 1;
            }
            sw.Start();
            float idx_f = 1;
            Long64_t evt_idx = 0;
            while (events) {
                auto count = branchF->GetEntriesFast(evt_idx, branchbuf);
                if (R__unlikely(count < 0)) {
                    printf("Failed to get entries via the 'fast' method for index %d.\n", evt_idx);
                    return 1;
                }
                if (events > count) {
                    events -= count;
                } else {
                    events = 0;
                }
                float *entry = reinterpret_cast<float*>(branchbuf.GetCurrent());
                for (Int_t idx=0; idx<count; idx++) {
                    idx_f++;
/*
                    Int_t *buf = reinterpret_cast<Int_t*>(&entry[idx]);
                    *buf = __builtin_bswap32(*buf);
*/

                    if (R__unlikely((evt_idx < 16000000) && (entry[idx] != idx_f))) {
                        printf("Incorrect value on myFloat branch: %f (event %ld)\n", entry[idx], evt_idx + idx);
                        return 1;
                    }
                }
                evt_idx += count;
            }
        }
        sw.Stop();
        printf("Successful read of all events.\n");
        printf("Total elapsed time (seconds) for bulk APIs: %.2f\n", sw.RealTime());
    } else {
        if (!do_std) {
            printf("There are currently no bulk APIs for writing.\n");
            return 1;
        }
        hfile = new TFile(fname, "RECREATE", "TTree float micro benchmark ROOT file");
        if (do_lz4) {
            hfile->SetCompressionLevel(7);  // High enough to get L4Z-HC
            hfile->SetCompressionAlgorithm(4);  // Enable LZ4 codec.
        } else if (do_uncompressed) {
            hfile->SetCompressionLevel(0); // No compression at all.
        } else if (do_zip) {
            hfile->SetCompressionLevel(6);
            hfile->SetCompressionAlgorithm(1);
        } else if (do_lzma) {
            hfile->SetCompressionLevel(6);
            hfile->SetCompressionAlgorithm(2); // LZMA
        }
        // Otherwise, we keep with the current ROOT defaults.
        tree = new TTree("T", "A ROOT tree of floats.");
        float f = 2;
        TBranch *branch2 = tree->Branch("myFloat", &f, 320000, 1);
        branch2->SetAutoDelete(kFALSE);
        for (Long64_t ev = 0; ev < events; ev++) {
          tree->Fill();
          f ++;
        }
        hfile = tree->GetCurrentFile();
        hfile->Write();
        tree->Print();
        printf("Successful write of all events.\n");
    }
    hfile->Close();

    return 0;
}
