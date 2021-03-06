#include "boxm2_block.h"
#include <boct/boct_bit_tree.h>
#include <vgl/vgl_intersection.h>
#include <vgl/vgl_distance.h>
//:
// \file
#include <boxm2/boxm2_util.h>

boxm2_block::boxm2_block(boxm2_block_id id, char* buff): version_(1)
{
  block_id_ = id;
  buffer_ = buff;
  this->b_read(buff);
  read_only_ = true;
  n_cells_ =this->recompute_num_cells();
}

boxm2_block::boxm2_block(boxm2_block_id id, boxm2_block_metadata data, char* buffer)
{
  version_ = data.version_;
  init_level_ = data.init_level_;
  max_level_  = data.max_level_;
  max_mb_     = int(data.max_mb_);
  sub_block_dim_ = data.sub_block_dim_;
  sub_block_num_ = data.sub_block_num_;
  local_origin_ = data.local_origin_;
  block_id_ = id;
  buffer_ = buffer;
  this->b_read(buffer_);
  read_only_ = true;
  n_cells_ =this->recompute_num_cells();
}

boxm2_block::boxm2_block(boxm2_block_metadata data)
{
  version_ = data.version_;
  block_id_ = data.id_;
  this->init_empty_block(data);
  read_only_ = false;  // make sure that it is written back to disc
  n_cells_ =this->recompute_num_cells();
}

unsigned boxm2_block::recompute_num_cells(){
  unsigned N = 0;
  for( const boxm2_block::uchar16* it= this->trees().begin();it!=this->trees().end(); it++){
      boct_bit_tree curr_tree( (unsigned char*) it->data_block(),this->max_level_);
      N += curr_tree.num_cells();
  }
  return N;
}

bool boxm2_block::b_read(char* buff)
{
  if (version_ == 1)
  {
    long bytes_read = 0;

    //0. first 8 bytes denote size
    vcl_memcpy(&byte_count_, buff, sizeof(byte_count_));
    bytes_read += sizeof(byte_count_);

    //1. read init level, max level, max mb
    vcl_memcpy(&init_level_, buff+bytes_read, sizeof(init_level_));
    bytes_read += sizeof(init_level_);
    vcl_memcpy(&max_level_, buff+bytes_read, sizeof(max_level_));
    bytes_read += sizeof(max_level_);
    vcl_memcpy(&max_mb_, buff+bytes_read, sizeof(max_mb_));
    bytes_read += sizeof(max_mb_);

    //2. read in sub block dimension, sub block num
    double dims[4];
    vcl_memcpy(&dims, buff+bytes_read, sizeof(dims));
    bytes_read += sizeof(dims);
    int    nums[4];
    vcl_memcpy(&nums, buff+bytes_read, sizeof(nums));
    bytes_read += sizeof(nums);
    sub_block_dim_ = vgl_vector_3d<double>(dims[0], dims[1], dims[2]);
    sub_block_num_ = vgl_vector_3d<unsigned>(nums[0], nums[1], nums[2]);

    //4. setup big arrays (3d block of trees)
    uchar16* treesBuff = (uchar16*) (buff+bytes_read);
    trees_     = boxm2_array_3d<uchar16>( sub_block_num_.x(),
                                          sub_block_num_.y(),
                                          sub_block_num_.z(),
                                          treesBuff);
    bytes_read += sizeof(uchar16)*sub_block_num_.x()*sub_block_num_.y()*sub_block_num_.z();
    return true;
  }
  else if (version_ == 2)
  {
    uchar16* treesBuff = (uchar16*) (buff);
    byte_count_ = sizeof(uchar16)* sub_block_num_.x()*sub_block_num_.y()*sub_block_num_.z();
    trees_     = boxm2_array_3d<uchar16>( sub_block_num_.x(),
                                          sub_block_num_.y(),
                                          sub_block_num_.z(),
                                          treesBuff);
    return true;
  }
  else
    return false;
}

//:
//  This type of writing is sort of counter intuitive, as the buffer
//  just needs to be returned and written to disk. The first few calls
//  ensure the meta data is lined up correctly.  To use this, just pass in
//  the boxm2_block buffer.
bool boxm2_block::b_write(char* buff)
{
  long bytes_written = 0;

  if (version_ == 1)
  {
    //0. writing total size
    vcl_memcpy(buff, &byte_count_, sizeof(byte_count_));
    bytes_written += sizeof(byte_count_);

    //1. write init level, max level, max mb
    vcl_memcpy(buff+bytes_written, &init_level_, sizeof(init_level_));
    bytes_written += sizeof(init_level_);
    vcl_memcpy(buff+bytes_written, &max_level_, sizeof(max_level_));
    bytes_written += sizeof(max_level_);
    vcl_memcpy(buff+bytes_written, &max_mb_, sizeof(max_mb_));
    bytes_written += sizeof(max_mb_);

    //2. Write sub block dimension, sub block num
    double dims[4] = {sub_block_dim_.x(), sub_block_dim_.y(), sub_block_dim_.z(), 0.0};
    vcl_memcpy(buff+bytes_written, dims, 4 * sizeof(double));
    bytes_written += 4 * sizeof(double);

    unsigned int nums[4] = {sub_block_num_.x(), sub_block_num_.y(), sub_block_num_.z(), 0 };
    vcl_memcpy(buff+bytes_written, nums, 4 * sizeof(unsigned int));
    bytes_written += 4 * sizeof(int);
  }
  //the arrays themselves should be already in the char buffer, so no need to copy
  return true;
}


//: initializes empty scene given
// This method uses the max_mb parameter to determine how many data cells to
// allocate.  MAX_MB is assumed to include blocks, alpha, mog3, num_obs and 16 byte aux data
bool boxm2_block::init_empty_block(boxm2_block_metadata data)
{
#if 0 // unused constants
  //calc max number of bytes, data buffer length, and alpha init (consts)
  const int MAX_BYTES    = int(data.max_mb_)*1024*1024;
  const int BUFF_LENGTH  = 1L<<16; // 65536
#endif

  //total number of (sub) blocks in the scene
  int total_blocks =  data.sub_block_num_.x()
                    * data.sub_block_num_.y()
                    * data.sub_block_num_.z();

  //to initialize
  int num_buffers, blocks_per_buffer;

  //only 1 buffer, blocks per buffer is all blocks
  num_buffers = 1;
  blocks_per_buffer = total_blocks;
  vcl_cout<<"Num buffers: "<<num_buffers
          <<" .. num_trees: "<<blocks_per_buffer<<vcl_endl;

  //now construct a byte stream, and read in with b_read
  byte_count_ = calc_byte_count(num_buffers, blocks_per_buffer, total_blocks);
  init_level_ = data.init_level_;
  max_level_  = data.max_level_;
  max_mb_     = int(data.max_mb_);
  local_origin_ = data.local_origin_;
  buffer_ = new char[byte_count_];

  //get member variable metadata straight, then write to the buffer
  long bytes_read = 0;

//double dims[4];
  int nums[4];

  if (version_==1)
  {
  bytes_read += sizeof(byte_count_);   //0. first 8 bytes denote size
  bytes_read += sizeof(init_level_);   //1. read init level, max level, max mb
  bytes_read += sizeof(max_level_);
  bytes_read += sizeof(max_mb_);
//bytes_read += sizeof(dims);          //2. read in sub block dimension, sub block num
  bytes_read += sizeof(nums);
  }
  sub_block_dim_ = data.sub_block_dim_;
  sub_block_num_ = data.sub_block_num_;

  //4. setup big arrays (3d block of trees)
  uchar16* treesBuff = (uchar16*) (buffer_+bytes_read);
  trees_     = boxm2_array_3d<uchar16>( sub_block_num_.x(),
                                        sub_block_num_.y(),
                                        sub_block_num_.z(),
                                        treesBuff);
  bytes_read += sizeof(uchar16)*sub_block_num_.x()*sub_block_num_.y()*sub_block_num_.z();

  //--- Now initialize blocks and their pointers --------- ---------------------
  //6. initialize blocks in order
  int tree_index = 0;
  boxm2_array_3d<uchar16>::iterator iter;
  for (iter = trees_.begin(); iter != trees_.end(); ++iter)
  {
    //initialize empty tree
    uchar16 treeBlk( (unsigned char) 0 );

    //store root data index in bits [10, 11, 12, 13] ;
    treeBlk[10] = (tree_index) & 0xff;
    treeBlk[11] = (tree_index>>8)  & 0xff;
    treeBlk[12] = (tree_index>>16) & 0xff;
    treeBlk[13] = (tree_index>>24) & 0xff;

    //Set Init_Level, 1=just root, 2=2 generations, 3=3 generations, 4=all four
    if (init_level_== 1) {
      treeBlk[0] = 0;
      ++tree_index;
    }
    else if (init_level_ == 2){
      treeBlk[0] = 1;
      tree_index += 9;                //root + 1st
    }
    else if (init_level_ == 3) {
      treeBlk[0] = 1;
      treeBlk[1] = 0xff;
      tree_index += 1 + 8 + 64;       //root + 1st + 2nd
    }
    else if (init_level_ == 4) {
      treeBlk[0] = 1;
      for (int i=1; i<1+9; ++i)
        treeBlk[i] = 0xff;
      tree_index += 1 + 8 + 64 + 512; // root + 1st + 2nd + 3rd...
    }

    //store this tree in block bytes
    for (int i=0; i<16; i++)
      (*iter)[i] = treeBlk[i];
  }
  return true;
}


//: Given number of buffers, number of trees in each buffer, and number of total trees (x*y*z number)
// \return size of byte stream
long boxm2_block::calc_byte_count(int num_buffers, int trees_per_buffer, int num_trees)
{
  long toReturn = num_trees * sizeof(uchar16) ;
  if (version_ == 1)
  {
    toReturn += num_buffers*trees_per_buffer * sizeof(int)     //tree pointers
              + num_buffers*(sizeof(ushort) + sizeof(ushort2)) //blocks in buffers and mem ptrs
              + sizeof(long)                                   // this number
              + 3*sizeof(int)                                  // init level, max level, max_mb
              + 4*sizeof(double)                               // dims
              + 4*sizeof(int)                                  // nums
              + sizeof(int) + sizeof(int)                      // numBuffers, treeLen
    ;
  }
  return toReturn;
}
vgl_box_3d<double> boxm2_block::bounding_box_global() const{
  vgl_vector_3d<double> diag(sub_block_dim_.x()*sub_block_num_.x(),
                             sub_block_dim_.y()*sub_block_num_.y(),
                             sub_block_dim_.z()*sub_block_num_.z());
  return vgl_box_3d<double>(local_origin_,local_origin_+diag);
}

bool boxm2_block::contains(vgl_point_3d<double> const& global_pt, vgl_point_3d<double>& local_pt) const{
  vgl_box_3d<double> bbox = this->bounding_box_global();
  if(bbox.contains(global_pt.x(), global_pt.y(), global_pt.z())) {
    double local_x=(global_pt.x()-local_origin_.x())/sub_block_dim_.x();
    double local_y=(global_pt.y()-local_origin_.y())/sub_block_dim_.y();
    double local_z=(global_pt.z()-local_origin_.z())/sub_block_dim_.z();
    local_pt.set(local_x, local_y, local_z);
    return true;
  }
  return false;
}
bool boxm2_block::contains(vgl_point_3d<double> const& global_pt, vgl_point_3d<int>& local_pt) const{
  vgl_point_3d<double> p;
  if(!this->contains(global_pt, p))
    return false;
  int index_x=(int)vcl_floor(p.x());
  int index_y=(int)vcl_floor(p.y());
  int index_z=(int)vcl_floor(p.z());
  local_pt.set(index_x, index_y, index_z);
  return true;
}

bool boxm2_block::contains(vgl_point_3d<double> const& global_pt, vgl_point_3d<double>& local_tree_coords,
                           vgl_point_3d<double>& cell_center, double& side_length) const{
  if(!this->contains(global_pt, local_tree_coords))
    return false;
  int index_x=(int)vcl_floor(local_tree_coords.x());
  int index_y=(int)vcl_floor(local_tree_coords.y());
  int index_z=(int)vcl_floor(local_tree_coords.z());
  vnl_vector_fixed<unsigned char,16> treebits=trees_(index_x,index_y,index_z);
  boct_bit_tree tree(treebits.data_block(),max_level_);
  int bit_index=tree.traverse(local_tree_coords);
  unsigned depth=tree.depth_at(bit_index);
  side_length=static_cast<float>(sub_block_dim_.x()/((float)(1<<depth)));
  cell_center = tree.cell_center(bit_index);
  cell_center.set(cell_center.x()+index_x, cell_center.y()+index_y,cell_center.z()+index_z);
  return true;
}

bool boxm2_block::data_index(vgl_point_3d<double> const& global_pt, unsigned& index, unsigned& depth, double& side_length) const{
  vgl_point_3d<double> loc;
  if(!this->contains(global_pt, loc))
    return false;
  int index_x=(int)vcl_floor(loc.x());
  int index_y=(int)vcl_floor(loc.y());
  int index_z=(int)vcl_floor(loc.z());
  //could be on block boundary so double check array bounds
  if(index_x<0||index_x>=trees_.get_row1_count()) return false;
  if(index_x<0||index_y>=trees_.get_row2_count()) return false;
  if(index_x<0||index_z>=trees_.get_row3_count()) return false;
  vnl_vector_fixed<unsigned char,16> treebits=trees_(index_x,index_y,index_z);
  boct_bit_tree tree(treebits.data_block(),max_level_);
  int bit_index=tree.traverse(loc);
  depth=tree.depth_at(bit_index);
  index=tree.get_data_index(bit_index,false);
  side_length=static_cast<float>(sub_block_dim_.x()/((float)(1<<depth)));
  return true;
}
bool boxm2_block::data_index(vgl_point_3d<double> const& global_pt, unsigned& index) const{
  unsigned depth;
  double cell_side_length;
  return this->data_index(global_pt, index, depth, cell_side_length);
}

vcl_vector<cell_info> boxm2_block::cells_in_box(vgl_box_3d<double> const& global_box){
  vcl_vector<cell_info> temp;
  vgl_box_3d<double> bbox = this->bounding_box_global();
  vgl_box_3d<double> inter = vgl_intersection<double>(global_box, bbox);
  if(inter.is_empty())
    return temp;
  vgl_point_3d<double> min_pt = inter.min_point();
  double local_x_min =(min_pt.x()-local_origin_.x())/sub_block_dim_.x();
  double local_y_min =(min_pt.y()-local_origin_.y())/sub_block_dim_.y();
  double local_z_min =(min_pt.z()-local_origin_.z())/sub_block_dim_.z();
  int index_x_min=(int)vcl_floor(local_x_min);
  if(index_x_min<0) index_x_min = 0;

  int index_y_min=(int)vcl_floor(local_y_min);
   if(index_y_min<0) index_y_min = 0;

  int index_z_min=(int)vcl_floor(local_z_min);
  if(index_z_min<0) index_z_min = 0;

  vgl_point_3d<double> max_pt = inter.max_point();
  double local_x_max =(max_pt.x()-local_origin_.x())/sub_block_dim_.x();
  double local_y_max =(max_pt.y()-local_origin_.y())/sub_block_dim_.y();
  double local_z_max =(max_pt.z()-local_origin_.z())/sub_block_dim_.z();
  int index_x_max=(int)vcl_floor(local_x_max);
  int nx = static_cast<int>(trees_.get_row1_count());
  if(index_x_max >=nx) index_x_max = nx-1;
  int index_y_max=(int)vcl_floor(local_y_max);
  int ny = static_cast<int>(trees_.get_row2_count());
   if(index_y_max >=ny) index_y_max = ny-1;
  int index_z_max=(int)vcl_floor(local_z_max);
  int nz = static_cast<int>(trees_.get_row3_count());
   if(index_z_max >=nz) index_z_max = nz-1;
  vgl_point_3d<double> loc;
  for(int iz = index_z_min; iz<=index_z_max; ++iz){
    for(int iy = index_y_min; iy<=index_y_max; ++iy){
      for(int ix = index_x_min; ix<=index_x_max; ++ix){
        cell_info ci;
        vnl_vector_fixed<unsigned char,16> treebits=trees_(ix,iy,iz);
        boct_bit_tree tree(treebits.data_block(),max_level_);
        vcl_vector<int> leafBits = tree.get_leaf_bits(0,max_level_);
        vcl_vector<int>::iterator iter;
        for (iter = leafBits.begin(); iter != leafBits.end(); ++iter) {
           int currBitIndex = (*iter);
           int currIdx = tree.get_data_index(currBitIndex); //data index
           int curr_depth = tree.depth_at(currBitIndex);

           vgl_point_3d<double> localCenter = tree.cell_center(currBitIndex);
           vgl_point_3d<double> cellCenter(localCenter.x() + ix, localCenter.y()+ iy, localCenter.z() + iz);


           ci.depth_= curr_depth;
           ci.data_index_=currIdx;
           ci.side_length_= static_cast<float>(sub_block_dim_.x()/((float)(1<<ci.depth_)));
           ci.cell_center_.set(cellCenter.x() * this->sub_block_dim_.x() +local_origin_.x(),
                            cellCenter.y() * this->sub_block_dim_.y() +local_origin_.y(),
                            cellCenter.z() * this->sub_block_dim_.z()+local_origin_.z());
           temp.push_back(ci);
        }
      }
    }
  }
  return temp;
}

vcl_vector<vgl_point_3d<double> > boxm2_block::neighbors(vgl_point_3d<double> const& probe, double distance) const{
  vcl_vector<vgl_point_3d<double> > ret;
  double r = sub_block_dim_.x();//assume cubical tree subblock
  double dr = vcl_floor(distance/r) + r;// add r as margin for roundoff
  vgl_box_3d<double> bb = this->bounding_box_global();

  // find the voxel center (x0, y0, z0) containing the probe
  vgl_vector_3d<double> loc = (probe-local_origin_)/r;
  double x0 = vcl_floor(loc.x()), y0 = vcl_floor(loc.y()), z0 = vcl_floor(loc.z());
  x0 = x0*r + local_origin_.x();   y0 = y0*r + local_origin_.y();  z0 = z0*r + local_origin_.z();
  // scan the NxNxN neigborhood around the origin voxel
  for(double x = (x0-dr); x<=(x0+dr); x+=r)
    for(double y = (y0-dr); y<=(y0+dr); y+=r)
      for(double z = (z0-dr); z<=(z0+dr); z+=r){
        vgl_point_3d<double> p(x, y, z);//block cell of probe is also a neighbor
        if(!bb.contains(p)) // be sure neighbor is inside block
          continue;
        double d = vgl_distance(p, probe);
        if(d<=distance)
          ret.push_back(p);
      }
  return ret;
}
//------------ I/O -------------------------------------------------------------
vcl_ostream& operator <<(vcl_ostream &s, boxm2_block& block)
{
  return
  s << "Block ID=" << block.block_id() << '\n'
    << "Byte Count=" << block.byte_count() << '\n'
    << "Init level=" << block.init_level() << '\n'
    << "Max level=" << block.max_level() << '\n'
    << "Max MB=" << block.max_mb() << '\n'
    << "Sub Block Dim=" << block.sub_block_dim() << '\n'
    << "Sub Block Num=" << block.sub_block_num() << '\n'
    << "Local Origin " << block.local_origin() << vcl_endl;
}

//: Binary write boxm2_block to stream.
// DUMMY IMPLEMENTATION: does nothing!
void vsl_b_write(vsl_b_ostream&, boxm2_block_sptr const&) {}

//: Binary load boxm2_block from stream.
// DUMMY IMPLEMENTATION: does nothing!
void vsl_b_read(vsl_b_istream&, boxm2_block_sptr&) {}
//: Binary load boxm2_block from stream.
// DUMMY IMPLEMENTATION: does nothing!
void vsl_b_read(vsl_b_istream&, boxm2_block_sptr const&) {}
