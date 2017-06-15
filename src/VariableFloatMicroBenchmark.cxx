
#include <stdio.h>

#include "TBranch.h"
#include "TBufferFile.h"
#include "TFile.h"
#include "TTree.h"
#include "TStopwatch.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include "TTreeReaderArray.h"
#include "VariableLengthStruct.h"
#include "ROOT/TTreeReaderFast.hxx"
#include "ROOT/TTreeReaderValueFast.hxx"
#include "ROOT/TBulkBranchRead.hxx"

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
            TTreeReaderArray<float> myF(myReader, "myStruct.a");
            TTreeReaderValue<int> myI(myReader, "myStruct.myLen");
            Long64_t ev = 0;
            float idx_f = 0;
            sw.Start();
            while (myReader.Next()) {
                if (R__unlikely(ev == events)) {break;}
                if (R__unlikely(*myI != (ev % 10))) {
                   printf("Incorrect number of entries on myStruct.myLen branch: %d, expected %d (event %d)\n",
                          *myI, ev % 10, ev);
                }
                if (R__unlikely(myF.GetSize() != (ev % 10))) {
                   printf("Incorrect number of entries on myFloat branch: %d, expected %d (event %d)\n",
                          myF.GetSize(),
                          ev % 10,
                          ev);
                }
                for (int idx = 0; idx < *myI; idx++) {
                   float tree_f = myF[idx];
                   if (R__unlikely((ev < 16000000) && (tree_f != idx_f++))) {
                      printf("Incorrect value on myFloat branch: %f, expected %f (event %d, entry %d)\n",
                             tree_f, idx_f, ev, idx);
                   }
                }
                ev++;
            }
        } else if (do_fast_reader) {
            printf("Using faster reader APIs.\n");
            ROOT::Experimental::TTreeReaderFast myReader("T", hfile);
            ROOT::Experimental::TTreeReaderValueFast<float> myF(myReader, "myFloat");
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
            TBufferFile countbuf(TBuffer::kWrite, 32*1024);
            TTree *tree = dynamic_cast<TTree*>(hfile->Get("T"));
            if (!tree) {
                std::cout << "Failed to fetch tree named 'T' from input file.\n";
                return 1;
            }
            TBranch *branchF = tree->GetBranch("myStruct.a");
            if (!branchF) {
                std::cout << "Unable to find branch 'myStruct.a' in tree 'T'\n";
                return 1;
            }
            TBranch *branchG = tree->GetBranch("myStruct.myLen");
            if (!branchG) {
                std::cout << "Unable to find branch 'myStruct.myLen' in tree 'T'\n";
                return 1;
            }
            float idx_f = 0;
            Long64_t evt_idx = 0;
            while (events) {
                auto count = branchF->GetBulkRead().GetEntriesSerialized(evt_idx, branchbuf, &countbuf);
                if (R__unlikely(count < 0)) {
                    printf("Failed to get entries via the 'serialized' method for index %d.\n", evt_idx);
                    return 1;
                }
                if (events > count) {
                    events -= count;
                } else {
                    events = 0;
                }
                char *entry_buf = branchbuf.GetCurrent();
                int *entry_count_buf = reinterpret_cast<int*>(countbuf.GetCurrent());
                for (Int_t idx=0; idx<count; idx++) {
                    entry_buf++;  // First byte in an event is header.
                    int entry_count = __builtin_bswap32(entry_count_buf[idx]);
                    //printf("Event %lld has %d entries.\n", evt_idx+idx, entry_count);
                    if (R__unlikely(entry_count != ((evt_idx+idx) % 10))) {
                       printf("Incorrect number of entries on myStruct.myLen branch: %d, expected %d (event %d)\n",
                              entry_count, (evt_idx+idx) % 10, evt_idx + idx);
                       return 1;
                    }
                    for (int entry_idx=0; entry_idx<entry_count; entry_idx++) {
                        Int_t *buf = reinterpret_cast<Int_t*>(entry_buf);
                        *buf = __builtin_bswap32(*buf);
                        float entry_f;
                        memcpy(&entry_f, buf, sizeof(float));
                        //printf("Entry %lld (buffer %p) has value %.2f\n", evt_idx+idx, entry_buf, entry_f);
                        if (R__unlikely((evt_idx < 16000000) && (entry_f != idx_f))) {
                           printf("Incorrect value on myFloat branch: %d, expected %d (diff %f, event %ld)\n", entry_f, idx_f, fabs(entry_f-idx_f), evt_idx + idx);
                           return 1;
                        }
                        idx_f++;
                        entry_buf+=sizeof(float);
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
                auto count = branchF->GetBulkRead().GetEntriesFast(evt_idx, branchbuf);
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
        hfile = new TFile(fname, "RECREATE", "TTree variable-sized float array micro benchmark ROOT file");
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
        tree = new TTree("T", "A ROOT tree of variable-length structs.");
        float f_counter = 0;
        float f[10];
        double d[10];
        VariableLengthStruct vls;
        vls.a = f;
        vls.b = 0;
        vls.c = d;
        vls.myLen = 0;

        TBranch *branch2 = tree->Branch("myStruct.", &vls, 320000, 1);
        branch2->SetAutoDelete(kFALSE);
        for (Long64_t ev = 0; ev < events; ev++) {

          for (Int_t idx = 0; idx < (ev % 10); idx++) {
            f[idx] = f_counter++;
            d[idx] = f_counter + 1;
          }

          vls.myLen = ev % 10;
          vls.b++;
          tree->Fill();
        }
        hfile = tree->GetCurrentFile();
        hfile->Write();
        tree->Print();
        printf("Successful write of all events.\n");
    }
    hfile->Close();

    return 0;
}
