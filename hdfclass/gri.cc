//////////////////////////////////////////////////////////////////////////////
// 
// Copyright (c) 1996, California Institute of Technology.
//                     U.S. Government Sponsorship under NASA Contract
//                     NAS7-1260 is acknowledged.
// 
// Author: Isaac.C.Henry@jpl.nasa.gov
//
// $RCSfile: gri.cc,v $ - input stream class for HDF GR
// 
// $Log: gri.cc,v $
// Revision 1.7  2000/03/31 16:56:05  jimg
// Merged with release 3.1.4
//
// Revision 1.6.8.1  2000/03/20 23:26:07  jimg
// Removed debugging output
//
// Revision 1.6  1999/05/06 03:23:33  jimg
// Merged changes from no-gnu branch
//
// Revision 1.5  1999/05/05 23:33:43  jimg
// String --> string conversion
//
// Revision 1.4.6.1  1999/05/06 00:35:45  jimg
// Jakes String --> string changes
//
// Revision 1.4  1998/09/10 21:50:22  jehamby
// Fix subsetting for multi-component GR (Note: due to an HDF library bug,
// you can't actually subset a GR with >1 component, but at least retrieving
// the entire image works).  Also, remove debugging output and work around
// another HDF bug so palettes output as Byte instead of string.
//
// Revision 1.3  1998/07/13 20:26:35  jimg
// Fixes from the final test of the new build process
//
// Revision 1.2.4.1  1998/05/22 19:50:51  jimg
// Patch from Jake Hamby to support subsetting raster images
//
// Revision 1.2  1998/04/03 18:34:17  jimg
// Fixes for vgroups and Sequences from Jake Hamby
//
// Revision 1.1  1996/10/31 18:42:58  jimg
// Added.
//
// Revision 1.10  1996/09/20  20:25:48  ike
// Changed meta behavior for rasters. Palletes are now read when meta is true.
//
// Revision 1.9  1996/09/20  18:28:40  ike
// Fixed GR reading to read images in there native interace by default.
//
// Revision 1.8  1996/09/20  17:49:55  ike
// Added setinterlace() to set the interlace type for reading images.
// Used GRreqlutil to for palletes to be read in pixel interlace.
//
// Revision 1.7  1996/08/22  20:56:36  todd
// Corrected bug in destructor.  Cleaned up initializations.
//
// Revision 1.6  1996/07/22  17:15:09  todd
// Const-corrected hdfistream_gri::seek() routine.
//
// Revision 1.5  1996/06/18  22:06:19  ike
// Removed default argument values from argument lists.
//
// Revision 1.4  1996/06/18  01:13:20  ike
// Fixed infinite loop caused by problem incrementing _nattrs, _nfattrs.
// Added _ok(), !operator, and has_palette to hdf_gri.
//
// Revision 1.3  1996/06/18  00:57:13  todd
// Added hcerr.h include at top of module.
//
// Revision 1.2  1996/06/14  21:58:45  ike
// fixed operator>> to overwrite non vectors objects.
//
// Revision 1.1  1996/06/14  21:53:09  ike
// Initial revision
//
//////////////////////////////////////////////////////////////////////////////

#include <mfhdf.h>
#include <mfgr.h>

#include <string>
#include <vector>

#include <hcstream.h>
#include <hdfclass.h>
#include <hcerr.h>

// initialize a hdfistream_gri
void hdfistream_gri::_init(void) {
  _ri_id = _attr_index = _pal_index = 0;
  _npals = _nri = _nattrs = _nfattrs = _gr_id = _file_id = 0;
  _index = _interlace_mode = -1; _meta = _slab.set = false;
  return;
}

// retrieve descriptive information about the file containing RI
void hdfistream_gri::_get_fileinfo(void) {
  if(GRfileinfo(_gr_id, &_nri, &_nfattrs) < 0)
    THROW(hcerr_griinfo);
  return;
}

// retrieve information about open raster image
void hdfistream_gri::_get_iminfo(void) {
  char junk0[hdfclass::MAXSTR];
  int32 junk1, junk2, junk3, junk4[2];
  if ( GRgetiminfo(_ri_id, junk0, &junk1, &junk2, &junk3, junk4, &_nattrs ) < 0)
    THROW(hcerr_griinfo);
  else {  // find out how many palettes are associated with the image
    GRgetlutinfo(GRgetlutid(_ri_id,0), &junk1, &junk2, &junk3, &junk1);
    if ( junk2 == 0)
      _npals = 0;
    else
      _npals = 1;
  }
  return;
}

// end access to open raster image
void hdfistream_gri::_close_ri(void) {
  if (_ri_id != 0) {
    GRendaccess(_ri_id);
    _ri_id = _attr_index = _pal_index = _nattrs = 0;
    _index = -1;
  }
  return;
}

// constructor
hdfistream_gri::hdfistream_gri(const string filename) : hdfistream_obj(filename) {
  _init();
  if (_filename.length() != 0)
    open(_filename.c_str());
  return;
}

// open the GRI input stream
void hdfistream_gri::open(const char *filename) {
  if (filename == 0)              // no filename given
    THROW(hcerr_openfile);
  if (_file_id != 0)              // close any currently open file
    close();
  if ( (_file_id = Hopen((char *)filename, DFACC_RDONLY, 0)) < 0 )
    THROW(hcerr_openfile);
  _filename = filename;           // assign filename
  if ( (_gr_id = GRstart(_file_id) ) < 0)
    THROW(hcerr_openfile);
  _get_fileinfo();                // get file information
  rewind();                       // position at BOS to start
  return;
}

// close the stream, by ending the GRI interface and closing the file
void hdfistream_gri::close(void) {
  _close_ri();
  if (_gr_id != 0)
    GRend(_gr_id);
  if (_file_id != 0)
    Hclose(_file_id);
  _file_id = _gr_id = 0;
  _nri = _nfattrs = 0;
  return;
}

// position to the stream to the index'th image
void hdfistream_gri::seek(int index) {
  if (_filename.length() == 0) // no file open
    THROW(hcerr_invstream);
  _close_ri();
  _index = index;
  _ri_id = GRselect(_gr_id, _index);
  if (!eos() && !bos())
    _get_iminfo();
}

// position GRI stream to RI with name "name"
void hdfistream_gri::seek(const char *name) {
  if (_filename.length() == 0)  // no open file
    THROW(hcerr_invstream);
  int32 index = GRnametoindex(_gr_id, (char *)name);
  seek(index);
}

// position GRI stream to RI with reference "ref"
void hdfistream_gri::seek_ref(int ref) {
  if (_filename.length() == 0)  // no open file
    THROW(hcerr_invstream);
  int32 index = GRreftoindex(_gr_id, ref);
  seek(index);
}

// position GRI index in front of first RI
void hdfistream_gri::rewind(void) {
  if (_filename.length() == 0)  // no file open
    THROW(hcerr_invstream);
  _close_ri();                  // close any already open RI's
  _rewind();                    // seek to BOS
}

// check to see if stream is positioned in front of 
// the first RI in the file
bool hdfistream_gri::bos(void) const {
  if (_filename.length() == 0)  // no file open
    THROW(hcerr_invstream);
  if (_nri == 0)
    return false;
  if (_index == -1)
    return true;
  else
    return false;
}

// check to see if stream has been positioned
// past the last RI in the file
bool hdfistream_gri::eos(void) const {
  if (_filename.length() == 0)  // no file open
    THROW(hcerr_invstream);
  if (_nri == 0)
    return true;
  else {
    if (bos())
      return false;
    else
      return (_index >= _nri);
  }
}

// Check to see if stream is positioned past the last
// attribute in the currently open RI
bool hdfistream_gri::eo_attr(void) const {
  if (_filename.length() == 0)
    THROW(hcerr_invstream);
  if (eos())
    return true;
  else {
    if (bos())
      return (_attr_index >= _nfattrs);
    else
      return (_attr_index >= _nattrs);
  }
}

// Set interlace type for read into memory
void hdfistream_gri::setinterlace(int32 interlace_mode) {
  if ( interlace_mode == MFGR_INTERLACE_PIXEL ||
       interlace_mode == MFGR_INTERLACE_COMPONENT ||
       interlace_mode == MFGR_INTERLACE_LINE )
    _interlace_mode = interlace_mode;
  else
    THROW(hcerr_interlace);
}

// check to see if stream is positioned past the last palette
// in the currently open RI
bool hdfistream_gri::eo_pal(void) const {
  if (_filename.length() == 0)
    THROW(hcerr_invstream);
  if (eos())
    return true;
  else {
    if (bos())
      return true;
    else
      return (_pal_index >= _npals);
  }
}

// set slab parameters
void hdfistream_gri::setslab(vector<int> start, vector<int> edge, 
			     vector<int> stride, bool reduce_rank) {
    // check validity of input
    if (start.size() != edge.size()  ||  edge.size() != stride.size()  ||
	start.size() == 0)
	THROW(hcerr_invslab);

    if(start.size() == 3) {
      // erase # of components, if present:  only X and Y subsetting allowed
      start.erase(start.begin());
      edge.erase(edge.begin());
      stride.erase(stride.begin());
    }

    for (int i=0; i<2; ++i) {
	if (start[i] < 0)
	    THROW(hcerr_invslab);
	if (edge[i] <= 0)
	    THROW(hcerr_invslab);
	if (stride[i] <= 0)
	    THROW(hcerr_invslab);
	// swap the X and Y dimensions because DODS prints data in [y][x] form
	// but HDF wants dimensions in [x][y]
	_slab.start[1-i] = start[i];
	_slab.edge[1-i] = edge[i];
	_slab.stride[1-i] = stride[i];
    }
    _slab.set = true;
    _slab.reduce_rank = reduce_rank;
}

// read a single RI
hdfistream_gri & hdfistream_gri::operator>>(hdf_gri & hr){
  if (_filename.length() == 0) // no file open
    THROW(hcerr_invstream);  // is this the right thing to throw?
  // delete any prevous data in hr
  hr.palettes = vector<hdf_palette>();
  hr.attrs = vector<hdf_attr>();
  hr.image = hdf_genvec();
  hr.name = string();
  if (bos())
    seek(0);
  if (eos())
    return *this;
  // get basic info about RI
  char name[hdfclass::MAXSTR];
  int32 ncomp;
  int32 data_type;
  int32 il;
  int32 dim_sizes[2];
  int32 nattrs;
  if (GRgetiminfo(_ri_id, name, &ncomp, &data_type, &il, dim_sizes, &nattrs) < 0)
    THROW(hcerr_griinfo);
  hr.ref = GRidtoref(_ri_id);
  hr.name = name;
  hr.dims[0] = dim_sizes[0]; hr.dims[1] = dim_sizes[1];
  hr.num_comp = ncomp;
  if ( _interlace_mode == -1) {
    setinterlace(il);
    hr.interlace = il;
  }
  /* read in palettes */
  *this>>hr.palettes;
  *this>>hr.attrs;
  if (_meta)
    hr.image.import(data_type);
  else {
    int32 nelts;
    void *image;
    if (_slab.set) {
      nelts = _slab.edge[0] * _slab.edge[1] * ncomp;
      // allocate a temporary C array to hold the data from GRreadimage()
      int imagesize = nelts * DFKNTsize(data_type);
      image = (void *)new char[imagesize];
      if (image == 0)
	THROW(hcerr_nomemory);
      // read the image and store it in a hdf_genvec
      GRreqimageil(_ri_id,_interlace_mode);
#if 0
      cerr << "ncomp: " << ncomp << " imagesize: " << imagesize << endl;
      cerr << "_slab.start = " << _slab.start[0] << "," << _slab.start[1] <<
	" _slab.edge = " << _slab.edge[0] << "," << _slab.edge[1] <<
	" _slab.stride = " << _slab.stride[0] << "," << _slab.stride[1] << endl;
#endif
      if (GRreadimage(_ri_id, _slab.start, _slab.stride, _slab.edge, image) < 0) {
	delete []image;   // problem: clean up and throw an exception
	THROW(hcerr_griread);
      }
    } else {
      int32 zero[2];
      zero[0] = zero[1] = 0;
      nelts = dim_sizes[0] * dim_sizes[1] * ncomp;
      // allocate a temporary C array to hold the data from GRreadimage()
      int imagesize = nelts * DFKNTsize(data_type);
      image = (void *)new char[imagesize];
      if (image == 0)
	THROW(hcerr_nomemory);
      // read the image and store it in a hdf_genvec
      GRreqimageil(_ri_id,_interlace_mode);
#if 0
      cerr << "dim_sizes[0] = " << dim_sizes[0] << " dim_sizes[1] = "
	   << dim_sizes[1] << endl;
#endif
      if (GRreadimage(_ri_id, zero, 0, dim_sizes, image) < 0) {
	delete []image;   // problem: clean up and throw an exception
	THROW(hcerr_griread);
      }
    }
    // try { // try to import into an hdf_genvec
    hr.image.import(data_type, image, nelts);
    // }
    // catch(...)
    //    delete [] image;
    //    throw;
    // }
    delete [] image;  // deallocate temporary C array
  }
  seek_next();   // position to next RI
  return *this;
}  

// read in all the RI's in a file
hdfistream_gri & hdfistream_gri::operator>>(vector<hdf_gri> & hrv) {
  for (hdf_gri gri; !eos(); ) {
    *this>>gri;
    hrv.push_back(gri);
  }
  return *this;
}

// load attribute currently positioned at
hdfistream_gri & hdfistream_gri::operator>>(hdf_attr& ha) {
  if (_filename.length() == 0)
    THROW(hcerr_invstream);
  if (eo_attr())
    return *this;
  // prepare to read attribute information: set nattrs depending on whether
  // reading file attributes or GRI attributes
  int32 id;
  int nattrs;
  if (bos()) {
    nattrs = _nfattrs;
    id = _gr_id;
  }
  else {
    nattrs = _nattrs;
    id = _ri_id;
  }
  char name[hdfclass::MAXSTR];
  int32 number_type, count;
  if (GRattrinfo(id, _attr_index, name, &number_type, &count) < 0)
    THROW(hcerr_griinfo);
  // allocate a temporary C array to hold data from GRgetattr()
  void *data;
  data = (void *)new char[count*DFKNTsize(number_type)];
  if (data == 0)
    THROW(hcerr_nomemory);
  // read attribute values and store them in an hdf_genvec
  if (GRgetattr(id, _attr_index, data) < 0) {
    delete []data;  // problem: clean up
    THROW(hcerr_griinfo);
  }
  // eliminate trailing null characters from the data string
  // they cause GNU's string class problems
  if (number_type == DFNT_CHAR)
    count = (int32)min((int)count,(int)strlen((char *)data));
  // try { // try to allocate an hdf_genvec
  if (count > 0) {
    ha.values.import(number_type, data, count);
// }
// catch(...) { //problem allocating hdf_genvec: clean up and rethrow
//    delete  []data;
//    throw;
//  }
  }
  delete []data;
  ++_attr_index;
  ha.name = name;
  return *this;
}

// read in all of the GRI attributes for currently open RI
hdfistream_gri & hdfistream_gri::operator>>(vector<hdf_attr>& hav) {
  for (hdf_attr att; !eo_attr();) {
    *this>>att;
    hav.push_back(att);
  }
  _attr_index = 0;
  return *this;
}

hdfistream_gri & hdfistream_gri::operator>>(hdf_palette & hp) {
  if (_filename.length() == 0)  // no file open
    THROW(hcerr_invstream);
  if (eo_pal()) // if positioned past last dimension, do nothing
    return *this;
  // open palette currently positioned at and increment pal index
  int32 pal_id;
  if ( (pal_id = GRgetlutid(_ri_id, _pal_index)) < 0 )
    THROW(hcerr_griinfo);
  // get palette information;
  int32 ncomp=0, number_type=0, num_entries=0, junk0;
  if (GRgetlutinfo(pal_id, &ncomp, &number_type, &junk0, &num_entries) < 0)
    THROW(hcerr_griinfo);
  else {
    hp.ncomp = ncomp;
    hp.num_entries = num_entries;
  }
  // Note: due to a bug in the HDF library, the palette number type is returned
  // as DFNT_UCHAR8 instead of DFNT_UINT8.  We correct that here because
  // the current mapping for DFNT_UCHAR8 is string instead of Byte
  if(number_type==DFNT_UCHAR8)
    number_type = DFNT_UINT8;

  int32 count = ncomp*num_entries;
  if (number_type != 0) {  // found a palette
    void *pal_data;
    pal_data = (void *) new char[count*DFKNTsize(number_type)];
    if (pal_data == 0)
      THROW(hcerr_nomemory);
    // read the palette data and store it in an hdf_genvec
    GRreqlutil(pal_id, MFGR_INTERLACE_PIXEL);
    if (GRreadlut(pal_id, pal_data) < 0) {
      delete []pal_data;    // problem: clean up and
      THROW(hcerr_griinfo); // throw an exception
    }
    // try { // problem allocating hdf_genvec: clean up and rethrow
    hp.table.import(number_type, pal_data, count);
 // }
 // catch(...) { // problem allocating hdf_genvec: clean up and rethrow
 //    delete []data;
 //    throw;
 // }
    delete []pal_data;  // deallocating temporary C array
  }
  ++_pal_index;
  return *this;
}

// read in all of the RI palettes for the currently open RI
hdfistream_gri & hdfistream_gri::operator>>(vector<hdf_palette> & hpv) {
  for (hdf_palette pal; !eo_pal();) {
    *this>>pal;
    hpv.push_back(pal);
  }
  return *this;
}

// Verify that the hdf_gri is in an OK state (i.e., that it is initialized correctly)
// if user has passed a ptr to a bool then set that to whether there is a dimension
// scale
bool hdf_gri::_ok() const {
  // The image should have the same number of components as indicated
  // by the number of dimensions and number of componets
  bool ok = (dims[0]*dims[1]*num_comp == image.size());
  if (!ok) return (ok);
  if (has_palette())
    for(int i=0; i<int(palettes.size()) && ok; i++)
      ok = (palettes[i].ncomp*palettes[i].num_entries == palettes[i].table.size());
  return (ok);
}


