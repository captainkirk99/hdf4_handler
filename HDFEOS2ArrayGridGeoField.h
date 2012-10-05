/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the latitude and longitude of  the HDF-EOS2 Grid 
// There are two typical cases: 
// read the lat/lon from the file and calculate lat/lon using the EOS2 library.
// Several variations are also handled:
// 1. For geographic and Cylinderic Equal Area projections, condense 2-D to 1-D.
// 2. For some files, the longitude is within 0-360 range instead of -180 - 180 range.
// We need to convert 0-360 to -180-180.
// 3. Some files have fillvalues in the lat. and lon. for the geographic projection.
// 4. Several MODIS files don't have the correct parameters inside StructMetadata.
// We can obtain the starting point, the step and replace the fill value.
//  Authors:   MuQun Yang <myang6@hdfgroup.org> Choonghwan Lee
// Copyright (c) 2009-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////
#ifdef USE_HDFEOS2_LIB
#ifndef HDFEOS2ARRAY_GRIDGEOFIELD_H
#define HDFEOS2ARRAY_GRIDGEOFIELD_H

#include <Array.h>
#include "mfhdf.h"
#include "hdf.h"
#include "HdfEosDef.h"

using namespace libdap;

class HDFEOS2ArrayGridGeoField:public Array
{
  public:
  HDFEOS2ArrayGridGeoField (int rank, int fieldtype, bool llflag, bool ydimmajor, bool condenseddim, bool speciallon, int specialformat, const std::string & filename, const std::string & gridname, const std::string & fieldname, const string & n = "", BaseType * v = 0):
	Array (n, v),
		rank (rank),
		fieldtype (fieldtype),
		llflag (llflag),
		ydimmajor (ydimmajor),
		condenseddim (condenseddim),
		speciallon (speciallon),
		specialformat (specialformat),
		filename (filename), gridname (gridname), fieldname (fieldname) {
	}
	virtual ~ HDFEOS2ArrayGridGeoField ()
	{
	}
	int format_constraint (int *cor, int *step, int *edg);

	BaseType *ptr_duplicate ()
	{
		return new HDFEOS2ArrayGridGeoField (*this);
	}

	// Calculate Lat and Lon based on HDF-EOS2 library.
	void CalculateLatLon (int32 gfid, int fieldtype, int specialformat, float64 * outlatlon, int32 * offset, int32 * count, int32 * step, int nelms);

	// Calculate Special Latitude and Longitude.
	void CalculateSpeLatLon (int32 gfid, int fieldtype, float64 * outlatlon, int32 * offset, int32 * count, int32 * step, int nelms);

        // Calculate Latitude and Longtiude for the Geo-projection for very large number of elements per dimension.
        void CalculateLargeGeoLatLon(int32 gfid, int32 gridid,  int fieldtype, float64* latlon, int *start, int *count, int *step, int nelms);
	// test for undefined values returned by longitude-latitude calculation
	bool isundef_lat(double value)
	{
  		if(isinf(value)) return(true);
  		if(isnan(value)) return(true);
  		// GCTP_LAMAZ returns "1e+51" for values at the opposite poles
  		if(value < -90.0 || value > 90.0) return(true);
  		return(false);
	} // end bool isundef_lat(double value)

	bool isundef_lon(double value)
	{
  		if(isinf(value)) return(true);
  		if(isnan(value)) return(true);
  		// GCTP_LAMAZ returns "1e+51" for values at the opposite poles
  		if(value < -180.0 || value > 180.0) return(true);
  		return(false);
	} // end bool isundef_lat(double value)

	// Given rol, col address in double array of dimension YDim x XDim
	// return value of nearest neighbor to (row,col) which is not undefined
	double nearestNeighborLatVal(double *array, int row, int col, int YDim, int XDim)
	{
  		// test valid row, col address range
  		if(row < 0 || row > YDim || col < 0 || col > XDim)
  		{
    			cerr << "nearestNeighborLatVal("<<row<<", "<<col<<", "<<YDim<<", "<<XDim;
    			cerr <<"): index out of range"<<endl;
    			return(0.0);
  		}
  		// address (0,0)
  		if(row < YDim/2 && col < XDim/2)
  		{ /* search by incrementing both row and col */
    			if(!isundef_lat(array[(row+1)*XDim+col])) return(array[(row+1)*XDim+col]);
    			if(!isundef_lat(array[row*XDim+col+1])) return(array[row*XDim+col+1]);
    			if(!isundef_lat(array[(row+1)*XDim+col+1])) return(array[(row+1)*XDim+col+1]);
    			/* recurse on the diagonal */
    			return(nearestNeighborLatVal(array, row+1, col+1, YDim, XDim));
  		}
		if(row < YDim/2 && col > XDim/2)
  		{ /* search by incrementing row and decrementing col */
    			if(!isundef_lat(array[(row+1)*XDim+col])) return(array[(row+1)*XDim+col]);
    			if(!isundef_lat(array[row*XDim+col-1])) return(array[row*XDim+col-1]);
    			if(!isundef_lat(array[(row+1)*XDim+col-1])) return(array[(row+1)*XDim+col-1]);
    			/* recurse on the diagonal */
    			return(nearestNeighborLatVal(array, row+1, col-1, YDim, XDim));
  		}
  		if(row > YDim/2 && col < XDim/2)
  		{ /* search by incrementing col and decrementing row */
    			if(!isundef_lat(array[(row-1)*XDim+col])) return(array[(row-1)*XDim+col]);
    			if(!isundef_lat(array[row*XDim+col+1])) return(array[row*XDim+col+1]);
    			if(!isundef_lat(array[(row-1)*XDim+col+1])) return(array[(row-1)*XDim+col+1]);
     			/* recurse on the diagonal */
    			return(nearestNeighborLatVal(array, row-1, col+1, YDim, XDim));
  		}
  		if(row > YDim/2 && col > XDim/2)
  		{ /* search by decrementing both row and col */
    			if(!isundef_lat(array[(row-1)*XDim+col])) return(array[(row-1)*XDim+col]);
    			if(!isundef_lat(array[row*XDim+col-1])) return(array[row*XDim+col-1]);
    			if(!isundef_lat(array[(row-1)*XDim+col-1])) return(array[(row-1)*XDim+col-1]);
    			/* recurse on the diagonal */
    			return(nearestNeighborLatVal(array, row-1, col-1, YDim, XDim));
  		}
                // dummy return, turn off the compiling warning
                return 0.0;
	} // end

	double nearestNeighborLonVal(double *array, int row, int col, int YDim, int XDim)
	{
  		// test valid row, col address range
  		if(row < 0 || row > YDim || col < 0 || col > XDim)
  		{
    			cerr << "nearestNeighborLonVal("<<row<<", "<<col<<", "<<YDim<<", "<<XDim;
    			cerr <<"): index out of range"<<endl;
    			return(0.0);
  		}
  		// address (0,0)
  		if(row < YDim/2 && col < XDim/2)
  		{ /* search by incrementing both row and col */
    			if(!isundef_lon(array[(row+1)*XDim+col])) return(array[(row+1)*XDim+col]);
    			if(!isundef_lon(array[row*XDim+col+1])) return(array[row*XDim+col+1]);
    			if(!isundef_lon(array[(row+1)*XDim+col+1])) return(array[(row+1)*XDim+col+1]);
    			/* recurse on the diagonal */
    			return(nearestNeighborLonVal(array, row+1, col+1, YDim, XDim));
  		}
  		if(row < YDim/2 && col > XDim/2)
  		{ /* search by incrementing row and decrementing col */
    			if(!isundef_lon(array[(row+1)*XDim+col])) return(array[(row+1)*XDim+col]);
    			if(!isundef_lon(array[row*XDim+col-1])) return(array[row*XDim+col-1]);
    			if(!isundef_lon(array[(row+1)*XDim+col-1])) return(array[(row+1)*XDim+col-1]);
    			/* recurse on the diagonal */
    			return(nearestNeighborLonVal(array, row+1, col-1, YDim, XDim));
  		}
 		if(row > YDim/2 && col < XDim/2)
  		{ /* search by incrementing col and decrementing row */
    			if(!isundef_lon(array[(row-1)*XDim+col])) return(array[(row-1)*XDim+col]);
    			if(!isundef_lon(array[row*XDim+col+1])) return(array[row*XDim+col+1]);
    			if(!isundef_lon(array[(row-1)*XDim+col+1])) return(array[(row-1)*XDim+col+1]);
     			/* recurse on the diagonal */
    			return(nearestNeighborLonVal(array, row-1, col+1, YDim, XDim));
  		}
  		if(row > YDim/2 && col > XDim/2)
  		{ /* search by decrementing both row and col */
    			if(!isundef_lon(array[(row-1)*XDim+col])) return(array[(row-1)*XDim+col]);
    			if(!isundef_lon(array[row*XDim+col-1])) return(array[row*XDim+col-1]);
    			if(!isundef_lon(array[(row-1)*XDim+col-1])) return(array[(row-1)*XDim+col-1]);
    			/* recurse on the diagonal */
    			return(nearestNeighborLonVal(array, row-1, col-1, YDim, XDim));
  		}

                // dummy return, turn off the compiling warning
                return 0.0;
       
	} // end

	// Calculate Latitude and Longitude for SOM Projection.
	void CalculateSOMLatLon(int32, int*, int*, int*, int);

	// Calculate Latitude and Longitude for LAMAZ Projection.
	void CalculateLAMAZLatLon(int32, int, float64*, int*, int*, int*, int);

	// Subsetting the latitude and longitude.
	template <class T> void LatLon2DSubset (T* outlatlon, int ydim, int xdim,
						 T* latlon, int32 * offset, int32 * count,
						 int32 * step);

        // Handle latitude and longitude when having fill value for geographic projection 
        //template <class T> void HandleFillLatLon(T* total_latlon, T* latlon,bool ydimmajor,
        template <class T> void HandleFillLatLon(vector<T> total_latlon, T* latlon,bool ydimmajor,
                                                 int fieldtype, int32 xdim , int32 ydim,
                                                 int32* offset, int32* count, int32* step, int fv);

	// Corrected Latitude and longitude when the lat/lon has fill value case.
	template < class T > bool CorLatLon (T * latlon, int fieldtype, int elms,
										 int fv);

	// Converted longitude from 0-360 to -180-180.
	template < class T > void CorSpeLon (T * lon, int xdim);

	// Lat and Lon for GEO and CEA projections need to be condensed from 2-D to 1-D.
	// This function does this.
	void getCorrectSubset (int *offset, int *count, int *step,
						   int32 * offset32, int32 * count32, int32 * step32,
						   bool condenseddim, bool ydimmajor, int fieldtype,
						   int rank);

	// Helper function to handle the case that lat. and lon. contain fill value.
	template < class T > int findfirstfv (T * array, int start, int end,
										  int fillvalue);

	virtual bool read ();

  private:
        int rank,fieldtype;
	bool llflag, ydimmajor, condenseddim, speciallon;
        int specialformat;
	std::string filename, gridname, fieldname;
};
#endif
#endif
