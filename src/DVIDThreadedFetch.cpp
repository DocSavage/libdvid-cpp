#include <libdvid/DVIDThreadedFetch.h>
#include <libdvid/DVIDException.h>

#include <vector>
#include <boost/thread/thread.hpp>

using std::string;
using std::vector;

//! Max blocks to request at one tiem
static const int MAX_BLOCKS = 4096;

namespace libdvid {

struct FetchGrayBlocks {
    FetchGrayBlocks(DVIDNodeService& service_, string grayscale_name_,
            bool use_blocks_, int request_efficiency_, int start_, int count_,
            vector<vector<int> >* spans_, vector<BinaryDataPtr>* blocks_) :
            service(service_), grayscale_name(grayscale_name_),
            use_blocks(use_blocks_), request_efficiency(request_efficiency_),
            start(start_), count(count_), spans(spans_), blocks(blocks_) {}

    void operator()()
    {
        uint8* blockdata = 0;
        if ((request_efficiency == 1) && !use_blocks) {
            blockdata = new uint8[DEFBLOCKSIZE*DEFBLOCKSIZE*DEFBLOCKSIZE];
        }
        // iterate only for the threads parts 
        for (int index = start; index < (start+count); ++index) {
            // load span info
            vector<int> span = (*spans)[index];
            int xmin = span[0];
            int y = span[1];
            int z = span[2];
            int curr_runlength = span[3];
            int block_index = span[4];

            if (use_blocks) {
                // use block interface (currently most re-copy)
                vector<int> block_coords;
                block_coords.push_back(xmin);
                block_coords.push_back(y);
                block_coords.push_back(z);
                GrayscaleBlocks blocks2 = service.get_grayblocks(grayscale_name, block_coords, curr_runlength);
                for (int j = 0; j < curr_runlength; ++j) {
                    BinaryDataPtr ptr = BinaryData::create_binary_data((const char*)blocks2[j], DEFBLOCKSIZE*DEFBLOCKSIZE*DEFBLOCKSIZE);
                    (*blocks)[block_index] = ptr;
                    ++block_index;
                }
            } else {
                Dims_t dims;
                dims.push_back(DEFBLOCKSIZE*curr_runlength);
                dims.push_back(DEFBLOCKSIZE);
                dims.push_back(DEFBLOCKSIZE);
                vector<int> offset;
                offset.push_back(xmin*DEFBLOCKSIZE);
                offset.push_back(y*DEFBLOCKSIZE);
                offset.push_back(z*DEFBLOCKSIZE);

                Grayscale3D grayvol = service.get_gray3D(grayscale_name,
                        dims, offset, false); 

                if (curr_runlength == 1) {
                    // do a simple copy for just one block
                    (*blocks)[block_index] = grayvol.get_binary();
                    ++block_index;
                } else {
                    const uint8* raw_data = grayvol.get_raw();

                    // otherwise create a buffer and do something more complicated 
                    for (int j = 0; j < curr_runlength; ++j) {
                        int offsetx = j * DEFBLOCKSIZE;
                        int offsety = curr_runlength*DEFBLOCKSIZE;
                        int offsetz = curr_runlength*DEFBLOCKSIZE*DEFBLOCKSIZE;
                        uint8* mod_data_iter = blockdata; 

                        for (int ziter = 0; ziter < DEFBLOCKSIZE; ++ziter) {
                            const uint8* data_iter = raw_data + ziter * offsetz;    
                            data_iter += (offsetx);
                            for (int yiter = 0; yiter < DEFBLOCKSIZE; ++yiter) {
                                for (int xiter = 0; xiter < DEFBLOCKSIZE; ++xiter) {
                                    *mod_data_iter = *data_iter;
                                    ++mod_data_iter;
                                    ++data_iter;
                                }
                                data_iter += ((offsety) - DEFBLOCKSIZE);
                            }
                        }
                        BinaryDataPtr ptr = BinaryData::create_binary_data((const char*) blockdata, DEFBLOCKSIZE*DEFBLOCKSIZE*DEFBLOCKSIZE);
                        (*blocks)[block_index] = ptr;
                        ++block_index;
                    }
                }
            }
        }

        if (blockdata) {
            delete []blockdata;
        }
    }


    DVIDNodeService service;
    string grayscale_name;
    bool use_blocks;
    int request_efficiency;
    int start; int count;
    vector<vector<int> >* spans;
    vector<BinaryDataPtr>* blocks;
};

struct FetchLabelBlocks {
    FetchLabelBlocks(DVIDNodeService& service_, string labelsname_, int start_, int count_,
            vector<vector<int> >* spans_, vector<BinaryDataPtr>* blocks_) :
            service(service_), labelsname(labelsname_), start(start_), count(count_),
            spans(spans_), blocks(blocks_) {}

    void operator()()
    {
        uint64* blockdata = 0;
        blockdata = new uint64[DEFBLOCKSIZE*DEFBLOCKSIZE*DEFBLOCKSIZE];
        // iterate only for the threads parts 
        for (int index = start; index < (start+count); ++index) {
            // load span info
            vector<int> span = (*spans)[index];
            int xmin = span[0];
            int y = span[1];
            int z = span[2];
            int curr_runlength = span[3];
            int block_index = span[4];

            Dims_t dims;
            dims.push_back(DEFBLOCKSIZE*curr_runlength);
            dims.push_back(DEFBLOCKSIZE);
            dims.push_back(DEFBLOCKSIZE);
            vector<int> offset;
            offset.push_back(xmin*DEFBLOCKSIZE);
            offset.push_back(y*DEFBLOCKSIZE);
            offset.push_back(z*DEFBLOCKSIZE);

            Labels3D labelvol = service.get_labels3D(labelsname,
                    dims, offset, false); 

            if (curr_runlength == 1) {
                // do a simple copy for just one block
                (*blocks)[block_index] = labelvol.get_binary();
                ++block_index;
            } else {
                const uint64* raw_data = labelvol.get_raw();

                // otherwise create a buffer and do something more complicated 
                for (int j = 0; j < curr_runlength; ++j) {
                    int offsetx = j * DEFBLOCKSIZE;
                    int offsety = curr_runlength*DEFBLOCKSIZE;
                    int offsetz = curr_runlength*DEFBLOCKSIZE*DEFBLOCKSIZE;
                    uint64* mod_data_iter = blockdata; 

                    for (int ziter = 0; ziter < DEFBLOCKSIZE; ++ziter) {
                        const uint64* data_iter = raw_data + ziter * offsetz;    
                        data_iter += (offsetx);
                        for (int yiter = 0; yiter < DEFBLOCKSIZE; ++yiter) {
                            for (int xiter = 0; xiter < DEFBLOCKSIZE; ++xiter) {
                                *mod_data_iter = *data_iter;
                                ++mod_data_iter;
                                ++data_iter;
                            }
                            data_iter += ((offsety) - DEFBLOCKSIZE);
                        }
                    }
                    BinaryDataPtr ptr = BinaryData::create_binary_data((const char*) blockdata,
                            sizeof(uint64)*DEFBLOCKSIZE*DEFBLOCKSIZE*DEFBLOCKSIZE);
                    (*blocks)[block_index] = ptr;
                    ++block_index;
                }
            }
        }

        if (blockdata) {
            delete []blockdata;
        }
    }


    DVIDNodeService service;
    string labelsname;
    int start; int count;
    vector<vector<int> >* spans;
    vector<BinaryDataPtr>* blocks;
};

struct WriteLabelBlocks {
    WriteLabelBlocks(DVIDNodeService& service_, string labelsname_, int start_, int count_,
            vector<vector<int> >* spans_, const vector<BinaryDataPtr>* blocks_) :
            service(service_), labelsname(labelsname_), start(start_), count(count_),
            spans(spans_), blocks(blocks_) {}

    void operator()()
    {
        // iterate only for the threads parts 
        for (int index = start; index < (start+count); ++index) {
            // load span info
            vector<int> span = (*spans)[index];
            int xmin = span[0];
            int y = span[1];
            int z = span[2];
            int curr_runlength = span[3];
            int block_index = span[4];

            Dims_t dims;
            dims.push_back(DEFBLOCKSIZE*curr_runlength);
            dims.push_back(DEFBLOCKSIZE);
            dims.push_back(DEFBLOCKSIZE);
            vector<int> offset;
            offset.push_back(xmin*DEFBLOCKSIZE);
            offset.push_back(y*DEFBLOCKSIZE);
            offset.push_back(z*DEFBLOCKSIZE);

            uint64* blockdata = new uint64[DEFBLOCKSIZE*DEFBLOCKSIZE*DEFBLOCKSIZE*curr_runlength];

            // otherwise create a buffer and do something more complicated 
            for (int j = 0; j < curr_runlength; ++j) {
                int offsetx = j * DEFBLOCKSIZE;
                int offsety = curr_runlength*DEFBLOCKSIZE;
                int offsetz = curr_runlength*DEFBLOCKSIZE*DEFBLOCKSIZE;
                const uint64* copy_data_iter = (uint64*) (*blocks)[block_index]->get_raw();

                for (int ziter = 0; ziter < DEFBLOCKSIZE; ++ziter) {
                    uint64* data_iter = blockdata + ziter * offsetz;    
                    data_iter += (offsetx);
                    for (int yiter = 0; yiter < DEFBLOCKSIZE; ++yiter) {
                        for (int xiter = 0; xiter < DEFBLOCKSIZE; ++xiter) {
                            *data_iter = *copy_data_iter;
                            ++copy_data_iter;
                            ++data_iter;
                        }
                        data_iter += ((offsety) - DEFBLOCKSIZE);
                    }
                }
                ++block_index;
            }

            // actually put label volume
            Labels3D volume(blockdata, DEFBLOCKSIZE*DEFBLOCKSIZE*DEFBLOCKSIZE*curr_runlength, dims);
            service.put_labels3D(labelsname, volume, offset, false); 
            delete []blockdata;
        }
    }


    DVIDNodeService service;
    string labelsname;
    int start; int count;
    vector<vector<int> >* spans;
    const vector<BinaryDataPtr>* blocks;
};




struct FetchTiles {
    FetchTiles(DVIDNodeService& service_, Slice2D orientation_,
            string instance_,
            unsigned int scaling_, int start_, int count_,
            const vector<vector<int> >& tile_locs_array_,
            vector<BinaryDataPtr>& results_) :
            service(service_), orientation(orientation_), instance(instance_),
            scaling(scaling_),
            start(start_), count(count_), tile_locs_array(tile_locs_array_),
            results(results_) {}

    void operator()()
    {
        for (int i = start; i < (start+count); ++i) {
            results[i] = 
                service.get_tile_slice_binary(instance, orientation, scaling, tile_locs_array[i]);
        }     
    }

    DVIDNodeService service;
    Slice2D orientation;
    string instance;
    unsigned int scaling;
    int start; int count;
    const vector<vector<int> >& tile_locs_array;
    vector<BinaryDataPtr>& results;
};

/*!
 * Given a body ID, determines all the X contiguous spans
 * and packs into an array.
*/
int get_block_spans(DVIDNodeService& service, string labelvol_name,
        uint64 bodyid, vector<vector<int> >& spans, int request_efficiency = 1)
{
    vector<BlockXYZ> blockcoords;
    if (!service.get_coarse_body(labelvol_name, bodyid, blockcoords)) {
        throw ErrMsg("Body not found, no grayscale blocks could be retrieved");
    }

    int num_requests = 0;
   
    // !! probably unnecessary copying going on
    // iterate through block coords and call ND or blocks one by one or contig
    int xmin; 
    int curr_runlength = 0;
    int start_index = 0;
    for (unsigned int i = 0; i < blockcoords.size(); ++i) {
        int z = blockcoords[i].z;
        int y = blockcoords[i].y;
        int x = blockcoords[i].x;
        if (curr_runlength == 0) {
            xmin = x; 
        }
        curr_runlength += 1; 
       
        bool requestblocks = false;

        if (request_efficiency == 0) {
            // if fetching 1 by 1 always request
            requestblocks = true;
        } else if (curr_runlength == MAX_BLOCKS) {
            // if there are too many blocks to fetch
            requestblocks = true;  
        } else if (i == (blockcoords.size()-1)) {
            // if there are no more blocks fetch
            requestblocks = true;
        } else if (i < (blockcoords.size()-1)) {
            // if y or z are different or x is non-contiguous time to fetch
            if ((blockcoords[i+1].z != z) || (blockcoords[i+1].y != y) || 
                    (((blockcoords[i+1].x)) != (x+1))) {
                requestblocks = true;
            }
        }

        if (requestblocks) {
            ++num_requests;

            // load into queue
            vector<int> span;
            span.push_back(xmin);
            span.push_back(y);
            span.push_back(z);
            span.push_back(curr_runlength);
            span.push_back(start_index);
            start_index += curr_runlength;
            spans.push_back(span);
            curr_runlength = 0;
        }
    }

    return num_requests;
}


vector<BinaryDataPtr> get_body_blocks(DVIDNodeService& service, string labelvol_name,
        string grayscale_name, uint64 bodyid, int num_threads,
        bool use_blocks, int request_efficiency)
{
    vector<vector<int> > spans;
    vector<BinaryDataPtr> blocks;

    int num_requests = get_block_spans(service, labelvol_name, bodyid, spans, request_efficiency);

    // launch threads
    boost::thread_group threads;

    if (num_requests < num_threads) {
        num_threads = num_requests;
    }
    
    int num_blocks = 0;
    for (int i = 0; i < spans.size(); ++i) {
        num_blocks += spans[i][3];
    }
    blocks.resize(num_blocks);

    int incr = num_requests / num_threads;
    int start = 0;
    int count_check = 0;

    for (int i = 0; i < num_threads; ++i) {
        int count = incr;
        if (i == (num_threads-1)) {
            count = num_requests - start;
        }
        count_check += count;
        threads.create_thread(FetchGrayBlocks(service, grayscale_name,
                    use_blocks, request_efficiency, start, count, &spans, &blocks));
        start += incr;
    }
    threads.join_all();
    assert(count_check == num_requests);
    std::cout << "Performed " << num_requests << " requests" << std::endl;
    return blocks;
}

vector<BinaryDataPtr> get_body_labelblocks(DVIDNodeService& service, string labelvol_name,
        uint64 bodyid, string labelsname, vector<vector<int> >& spans, int num_threads)
{
    vector<BinaryDataPtr> blocks;
    int num_requests = spans.size();
    
    if (spans.empty()) {
        num_requests = get_block_spans(service, labelvol_name, bodyid, spans);
    }

    // launch threads
    boost::thread_group threads;

    if (num_requests < num_threads) {
        num_threads = num_requests;
    }
    int num_blocks = 0;
    for (int i = 0; i < spans.size(); ++i) {
        num_blocks += spans[i][3];
    }
    blocks.resize(num_blocks);

    int incr = num_requests / num_threads;
    int start = 0;
    int count_check = 0;

    for (int i = 0; i < num_threads; ++i) {
        int count = incr;
        if (i == (num_threads-1)) {
            count = num_requests - start;
        }
        count_check += count;
        threads.create_thread(FetchLabelBlocks(service, labelsname, start, count, &spans, &blocks));
        start += incr;
    }
    threads.join_all();
    assert(count_check == num_requests);
    std::cout << "Performed " << num_requests << " requests" << std::endl;
    return blocks;
}


void put_labelblocks(DVIDNodeService& service, std::string labelsname,
        const vector<BinaryDataPtr>& blocks,
        vector<vector<int> >& spans, int num_threads)
{
    // launch threads
    boost::thread_group threads;
    int num_requests = spans.size();

    if (num_requests < num_threads) {
        num_threads = num_requests;
    }

    int incr = num_requests / num_threads;
    int start = 0;
    int count_check = 0;

    for (int i = 0; i < num_threads; ++i) {
        int count = incr;
        if (i == (num_threads-1)) {
            count = num_requests - start;
        }
        count_check += count;
        threads.create_thread(WriteLabelBlocks(service, labelsname, start, count, &spans, &blocks));
        start += incr;
    }
    threads.join_all();
    assert(count_check == num_requests);
    std::cout << "Performed " << num_requests << " requests" << std::endl;
}


vector<BinaryDataPtr> get_tile_array_binary(DVIDNodeService& service,
        string datatype_instance, Slice2D orientation, unsigned int scaling,
        const vector<vector<int> >& tile_locs_array, int num_threads)
{
    if (!num_threads) {
        num_threads = tile_locs_array.size();
    }
    vector<BinaryDataPtr> results(tile_locs_array.size());
    
    // launch threads
    boost::thread_group threads;

    // not an optimal partitioning
    int incr = tile_locs_array.size() / num_threads;
    int start = 0;

    for (int i = 0; i < num_threads; ++i) {
        int count = incr;
        if (i == (num_threads-1)) {
            count = tile_locs_array.size() - start;
        }
        threads.create_thread(FetchTiles(service, orientation,
                    datatype_instance, scaling, start, count,
                    tile_locs_array, results));
        start += incr;
    }
    threads.join_all();

    return results;
}

}
