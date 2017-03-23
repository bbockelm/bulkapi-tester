
#include <stdio.h>

#include "TBranch.h"
#include "TBufferFile.h"
#include "TFile.h"
#include "TTree.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include "TTreeReaderFast.h"
#include "TTreeReaderValueFast.h"

int main(int argc, char *argv[]) {

    TFile *hfile;
    TTree *tree;

    // Handle all the argument parsing up front.
    if (argc != 5) {
        fprintf(stderr, "Usage: %s read|write bulk|standard events fname\n", argv[0]);
        return 1;
    }
    bool do_read;
    if (!strcmp(argv[1], "read")) {
        do_read = true;
    } else if (!strcmp(argv[1], "write")) {
        do_read = false;
    } else {
        fprintf(stderr, "Second argument must be either 'read' or 'write'\n");
        return 1;
    }
    bool do_fast_reader = false;
    bool do_std;
    if (!strcmp(argv[2], "standard")) {
        do_std = true;
    } else if (!strcmp(argv[2], "bulk")) {
        do_std = false;
    } else if (!strcmp(argv[2], "fastreader")) {
        do_std = false;
        do_fast_reader = true;
    } else {
        fprintf(stderr, "Third argument must be either 'bulk' or 'standard'\n");
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

        if (do_std) {
            printf("Using standard read APIs.\n");
            // Read via standard APIs.
            TTreeReader myReader("T", hfile);
            TTreeReaderValue<float> myF(myReader, "myFloat");
            Long64_t idx = 0;
            float idx_f = 1;
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
                    Int_t *buf = reinterpret_cast<Int_t*>(&entry[idx]);
                    *buf = __builtin_bswap32(*buf);

                    if (R__unlikely((evt_idx < 16000000) && (entry[idx] != idx_f))) {
                        printf("Incorrect value on myFloat branch: %f (event %ld)\n", entry[idx], evt_idx + idx);
                        return 1;
                    }
                }
                evt_idx += count;
            }
        }
        printf("Successful read of all events.\n");
    } else {
        if (!do_std) {
            printf("There are currently no bulk APIs for writing.\n");
            return 1;
        }
        hfile = new TFile(fname, "RECREATE", "TTree float micro benchmark ROOT file");
        hfile->SetCompressionLevel(0);  // Disable compression.
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
