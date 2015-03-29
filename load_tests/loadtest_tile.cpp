#include <libdvid/DVIDNodeService.h>
#include "ScopeTime.h"

#include <iostream>

using std::cerr; using std::cout; using std::endl;
using namespace libdvid;
using std::vector;

using std::string;

// assume all blocks are BLK_SIZE in each dimension
int BLK_SIZE = 32;
int NUM_FETCHES = 500;
int TILESIZE = 512;
int FETCHSIZE = 1024;

/*!
 * Exercises the nD GETs and PUTs for the labelblk datatype.
*/
int main(int argc, char** argv)
{
    if (argc != 7 && argc != 8) {
        cout << "Usage: <program> <server_name> <uuid> <tilename> <startx> <starty> <startz> <seg: opt>" << endl;
        return -1;
    }
    try {
        ScopeTime overall_time;
        
        DVIDNodeService dvid_node(argv[1], argv[2]);

        int xstart = atoi(argv[4]);
        int ystart = atoi(argv[5]);
        int zstart = atoi(argv[6]);

        if (xstart % TILESIZE) {
            xstart += (TILESIZE - (xstart % TILESIZE)); 
        }
        if (ystart % TILESIZE) {
            ystart += (TILESIZE - (ystart % TILESIZE)); 
        }
        int xstartm = xstart + TILESIZE / 2;
        int ystartm = ystart + TILESIZE / 2;

        cout << "Starting Location: " << xstart << ", " << ystart << ", " << zstart << endl;

        string tilename = argv[3];
        string segname = "";

        if (argc == 8) {
            segname = argv[7]; 
        }

        cout << "*** Tile Fetching (Single Thread 9 Tiles Per Slice) ***" << endl;

        vector<unsigned int> tilepos;
        tilepos.push_back(0);
        tilepos.push_back(0);
        tilepos.push_back(zstart);
        int total_bytes_read = 0;
        ScopeTime tiles_timer(false);
        for (int z = zstart; z < (zstart+NUM_FETCHES); ++z) {
            // fetch 9 tiles
            ScopeTime tile_timer(false);
            tilepos[2] = z;
            int bytes_read = 0;

            tilepos[0] = xstart / TILESIZE;
            tilepos[1] = ystart / TILESIZE;
            BinaryDataPtr dum1 = dvid_node.get_tile_slice_binary(tilename, XY, 1, tilepos); 
            bytes_read += dum1->length();            

            tilepos[0] += 1;
            BinaryDataPtr dum2 = dvid_node.get_tile_slice_binary(tilename, XY, 1, tilepos); 
            bytes_read += dum2->length();            

            tilepos[0] += 1;
            BinaryDataPtr dum3 = dvid_node.get_tile_slice_binary(tilename, XY, 1, tilepos); 
            bytes_read += dum3->length();            

            tilepos[0] -= 2;
            tilepos[1] += 1;
            BinaryDataPtr dum4 = dvid_node.get_tile_slice_binary(tilename, XY, 1, tilepos); 
            bytes_read += dum4->length();            

            tilepos[0] += 1;
            BinaryDataPtr dum5 = dvid_node.get_tile_slice_binary(tilename, XY, 1, tilepos); 
            bytes_read += dum5->length();            

            tilepos[0] += 1;
            BinaryDataPtr dum6 = dvid_node.get_tile_slice_binary(tilename, XY, 1, tilepos); 
            bytes_read += dum6->length();            

            tilepos[0] -= 2;
            tilepos[1] += 1;
            BinaryDataPtr dum7 = dvid_node.get_tile_slice_binary(tilename, XY, 1, tilepos); 
            bytes_read += dum7->length();            

            tilepos[0] += 1;
            BinaryDataPtr dum8 = dvid_node.get_tile_slice_binary(tilename, XY, 1, tilepos); 
            bytes_read += dum8->length();            

            tilepos[0] += 1;
            BinaryDataPtr dum9 = dvid_node.get_tile_slice_binary(tilename, XY, 1, tilepos); 
            bytes_read += dum9->length();            

            total_bytes_read += bytes_read;
            double read_time = tile_timer.getElapsed();
            cout << "Read " << bytes_read << " bytes (9 tiles) in " << read_time << " seconds" << endl;
        }
        double total_read_time = tiles_timer.getElapsed();
        cout << "Read " << total_bytes_read << " bytes (" << NUM_FETCHES << " tile planes) in " << total_read_time << " seconds" << endl;
        cout << "Frame rate: " << total_read_time / NUM_FETCHES * 1000 << " milliseconds" << endl;

        // size for gray and label fetch
        vector<unsigned int> start;
        start.push_back(xstartm); start.push_back(ystartm); start.push_back(zstart);
        Dims_t lsizes;
        lsizes.push_back(FETCHSIZE); lsizes.push_back(FETCHSIZE); lsizes.push_back(1);

        // **************** Segmentation Fetch Time ******************8
        if (segname != "") {
            int bytes_read = FETCHSIZE * FETCHSIZE * sizeof(uint64);
            total_bytes_read = bytes_read * NUM_FETCHES;
            ScopeTime label_timer(false);

            // retrieve the image volume and make sure it makes the posted volume
            for (int z = zstart; z < (zstart+NUM_FETCHES); ++z) {
                ScopeTime get_timer(false);
                start[2] = z;
                Labels3D labelcomp = dvid_node.get_labels3D(segname, lsizes, start);
                double read_time = get_timer.getElapsed();
                cout << "Read " << bytes_read << " bytes in " << read_time << " seconds" << endl;
            }
            total_read_time = label_timer.getElapsed();
            cout << "Read " << total_bytes_read << " bytes (" << NUM_FETCHES << " segmentation " 
                << FETCHSIZE << "x" << FETCHSIZE << " planes) in " << total_read_time << " seconds" << endl;
            cout << "Frame rate: " << total_read_time / NUM_FETCHES * 1000 << " milliseconds" << endl;
        }


        // **************** Grayscale Fetch Time ******************8
        int bytes_read = FETCHSIZE * FETCHSIZE;
        total_bytes_read = bytes_read * NUM_FETCHES;
        ScopeTime gray_timer(false);

        // retrieve the image volume and make sure it makes the posted volume
        for (int z = zstart; z < (zstart+NUM_FETCHES); ++z) {
            ScopeTime get_timer(false);
            start[2] = z;
            Grayscale3D graycomp = dvid_node.get_gray3D("grayscale", lsizes, start);
            double read_time = get_timer.getElapsed();
            cout << "Read " << bytes_read << " bytes in " << read_time << " seconds" << endl;
        }
        total_read_time = gray_timer.getElapsed();
        cout << "Read " << total_bytes_read << " bytes (" << NUM_FETCHES << " grayscale " 
            << FETCHSIZE << "x" << FETCHSIZE << " planes) in " << total_read_time << " seconds" << endl;
        cout << "Frame rate: " << total_read_time / NUM_FETCHES * 1000 << " milliseconds" << endl;

    } catch (std::exception& e) {
        cerr << e.what() << endl;
        return -1;
    }

    // ?! validate my decompression algorithm for PNG and JPEG


    return 0;
}