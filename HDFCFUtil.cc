#include "HDFCFUtil.h"
#include <BESDebug.h>
#include <BESLog.h>

#define SIGNED_BYTE_TO_INT32 1
using namespace std;
using namespace libdap;

bool 
HDFCFUtil::check_beskeys(const string key) {

    bool found = false;
    string doset ="";
    const string dosettrue ="true";
    const string dosetyes = "yes";

    TheBESKeys::TheKeys()->get_value( key, doset, found ) ;
    if( true == found ) {
        doset = BESUtil::lowercase( doset ) ;
        if( dosettrue == doset  || dosetyes == doset )
            return true;
    }
    return false;

}

// These memory clearing functions are not useful. May remove later.  KY 2013-02-13 
#if 0
void
HDFCFUtil::ClearMem (int32 * offset32, int32 * count32, int32 * step32, int *offset, int *count, int *step)
{
    if(offset32 != NULL)
        delete[]offset32;
    if(count32 != NULL) 
        delete[]count32;
    if(step32 != NULL) 
        delete[]step32;
    if(offset != NULL) 
        delete[]offset;
    if(count != NULL) 
        delete[]count;
    if(step != NULL) 
        delete[]step;
}

void
HDFCFUtil::ClearMem2 (int32 * offset32, int32 * count32, int32 * step32)
{
    if(offset32 != NULL)
        delete[]offset32;
    if(count32 != NULL) 
        delete[]count32;
    if(step32 != NULL)
        delete[]step32;
}

void
HDFCFUtil::ClearMem3 (int *offset, int *count, int *step)
{
    if (offset != NULL)
        delete[]offset;
    if (count != NULL)
        delete[]count;
    if (step != NULL)
        delete[]step;
}
#endif

void 
HDFCFUtil::Split(const char *s, int len, char sep, std::vector<std::string> &names)
{
    names.clear();
    for (int i = 0, j = 0; j <= len; ++j) {
        if ((j == len && len) || s[j] == sep) {
            string elem(s + i, j - i);
            names.push_back(elem);
            i = j + 1;
            continue;
        }
    }
}

void 
HDFCFUtil::Split(const char *sz, char sep, std::vector<std::string> &names)
{
    Split(sz, (int)strlen(sz), sep, names);
}

bool
HDFCFUtil::insert_map(std::map<std::string,std::string>& m, string key, string val)
{
    pair<map<string,string>::iterator, bool> ret;
    ret = m.insert(make_pair(key, val));
    if(ret.second == false){
        m.erase(key);
        ret = m.insert(make_pair(key, val));
        if(ret.second == false){
            BESDEBUG("h4","insert_map():insertion failed on Key=" << key << " Val=" << val << endl);
        }
    }
    return ret.second;
}

string
HDFCFUtil::get_CF_string(string s)
{

    if(""==s) return s;
    string insertString(1,'_');

    // Always start with _ if the first character is not a letter
    if (true == isdigit(s[0]))
        s.insert(0,insertString);

    // New name conventions drop the first '/' from the path.
    if ('/' ==s[0])
        s.erase(0,1);

    for(unsigned int i=0; i < s.length(); i++)
        if((false == isalnum(s[i])) &&  (s[i]!='_'))
            s[i]='_';

    return s;

}

void 
HDFCFUtil::gen_unique_name(string &str,set<string>& namelist, int&clash_index) {

    pair<set<string>::iterator,bool> ret;
    string newstr = "";
    stringstream sclash_index;
    sclash_index << clash_index;
    newstr = str + sclash_index.str();

    ret = namelist.insert(newstr);
    if (false == ret.second) {
        clash_index++;
        gen_unique_name(str,namelist,clash_index);
    }
    else
        str = newstr;
}

void
HDFCFUtil::Handle_NameClashing(vector<string>&newobjnamelist,set<string>&objnameset) {

    //set<string> objnameset;
    pair<set<string>::iterator,bool> setret;
    set<string>::iterator iss;

    vector<string> clashnamelist;
    vector<string>::iterator ivs;

    map<int,int> cl_to_ol;
    int ol_index = 0;
    int cl_index = 0;

    vector<string>::const_iterator irv;

    for (irv = newobjnamelist.begin(); irv != newobjnamelist.end(); ++irv) {
        setret = objnameset.insert((*irv));
        if (false == setret.second ) {
            clashnamelist.insert(clashnamelist.end(),(*irv));
            cl_to_ol[cl_index] = ol_index;
            cl_index++;
        }
        ol_index++;
    }

    // Now change the clashed elements to unique elements; 
    // Generate the set which has the same size as the original vector.
    for (ivs=clashnamelist.begin(); ivs!=clashnamelist.end(); ivs++) {
        int clash_index = 1;
        string temp_clashname = *ivs +'_';
        HDFCFUtil::gen_unique_name(temp_clashname,objnameset,clash_index);
        *ivs = temp_clashname;
    }

    // Now go back to the original vector, make it unique.
    for (unsigned int i =0; i <clashnamelist.size(); i++)
        newobjnamelist[cl_to_ol[i]] = clashnamelist[i];

}

void
HDFCFUtil::Handle_NameClashing(vector<string>&newobjnamelist) {

    set<string> objnameset;
    Handle_NameClashing(newobjnamelist,objnameset);
}

// Borrowed codes from ncdas.cc in netcdf_handle4 module.
string
HDFCFUtil::print_attr(int32 type, int loc, void *vals)
{
    ostringstream rep;

    union {
        char *cp;
        short *sp;
        unsigned short *usp;
        int32 /*nclong*/ *lp;
        unsigned int *ui;
        float *fp;
        double *dp;
    } gp;

    switch (type) {

    case DFNT_UINT8:
    case DFNT_INT8:
        {
            unsigned char uc;
            gp.cp = (char *) vals;

            uc = *(gp.cp+loc);
            rep << (int)uc;
            return rep.str();
        }

    case DFNT_CHAR:
        {
            return escattr(static_cast<const char*>(vals));
        }

    case DFNT_INT16:
        {
            gp.sp = (short *) vals;
            rep << *(gp.sp+loc);
            return rep.str();
        }

    case DFNT_UINT16:
        {
            gp.usp = (unsigned short *) vals;
            rep << *(gp.usp+loc);
            return rep.str();
        }

    case DFNT_INT32:
        {
            gp.lp = (int32 *) vals;
            rep << *(gp.lp+loc);
            return rep.str();
        }

    case DFNT_UINT32:
        {
            gp.ui = (unsigned int *) vals;
            rep << *(gp.ui+loc);
            return rep.str();
        }

    case DFNT_FLOAT:
        {
            gp.fp = (float *) vals;
            rep << showpoint;
            rep << setprecision(10);
            rep << *(gp.fp+loc);
            if (rep.str().find('.') == string::npos
                && rep.str().find('e') == string::npos)
                rep << ".";
            return rep.str();
        }

    case DFNT_DOUBLE:
        {
            gp.dp = (double *) vals;
            rep << std::showpoint;
            rep << std::setprecision(17);
            rep << *(gp.dp+loc);
            if (rep.str().find('.') == string::npos
                && rep.str().find('e') == string::npos)
                rep << ".";
            return rep.str();
            break;
        }
    default:
        return string("UNKNOWN");
    }

}

string
HDFCFUtil::print_type(int32 type)
{

    // I expanded the list based on libdap/AttrTable.h.
    static const char UNKNOWN[]="Unknown";
    static const char BYTE[]="Byte";
    static const char INT16[]="Int16";
    static const char UINT16[]="UInt16";
    static const char INT32[]="Int32";
    static const char UINT32[]="UInt32";
    static const char FLOAT32[]="Float32";
    static const char FLOAT64[]="Float64";
    static const char STRING[]="String";

    // I got different cases from hntdefs.h.
    switch (type) {

    case DFNT_CHAR:
        return STRING;

    case DFNT_UCHAR8:
        return STRING;

    case DFNT_UINT8:
        return BYTE;

    case DFNT_INT8:
// ADD the macro
    {
#ifndef SIGNED_BYTE_TO_INT32
        return BYTE;
#else
        return INT32;
#endif
    }
    case DFNT_UINT16:
        return UINT16;

    case DFNT_INT16:
        return INT16;

    case DFNT_INT32:
        return INT32;

    case DFNT_UINT32:
        return UINT32;

    case DFNT_FLOAT:
        return FLOAT32;

    case DFNT_DOUBLE:
        return FLOAT64;

    default:
        return UNKNOWN;
    }

}


// Subset of latitude and longitude to follow the parameters from the DAP expression constraint
// Somehow this function doesn't work. Now it is not used. Still keep it here for the future investigation.
template < typename T >
void HDFCFUtil::LatLon2DSubset (T * outlatlon,
                                int majordim,
                                int minordim,
                                T * latlon,
                                int32 * offset,
                                int32 * count,
                                int32 * step)
{

    //  float64 templatlon[majordim][minordim];
    T (*templatlonptr)[majordim][minordim] = (typeof templatlonptr) latlon;
    int i, j, k;

    // do subsetting
    // Find the correct index
    int dim0count = count[0];
    int dim1count = count[1];
    int dim0index[dim0count], dim1index[dim1count];

    for (i = 0; i < count[0]; i++)      // count[0] is the least changing dimension 
        dim0index[i] = offset[0] + i * step[0];


    for (j = 0; j < count[1]; j++)
        dim1index[j] = offset[1] + j * step[1];

    // Now assign the subsetting data
    k = 0;

    for (i = 0; i < count[0]; i++) {
        for (j = 0; j < count[1]; j++) {
            outlatlon[k] = (*templatlonptr)[dim0index[i]][dim1index[j]];
            k++;

        }
    }
}

void HDFCFUtil::correct_fvalue_type(AttrTable *at,int32 dtype) {

    AttrTable::Attr_iter it = at->attr_begin();
    bool find_fvalue = false;
    while (it!=at->attr_end() && false==find_fvalue) {
        if (at->get_name(it) =="_FillValue")
        {                            
            find_fvalue = true;
            string fillvalue =""; 
            string fillvalue_type ="";
            fillvalue =  (*at->get_attr_vector(it)->begin());
            fillvalue_type = at->get_type(it);
//cerr<<"fillvalue type is "<<fillvalue_type <<endl;
            string var_type = HDFCFUtil::print_type(dtype);
//cerr<<"var type is "<<var_type <<endl;
            if(fillvalue_type != var_type){ 
                at->del_attr("_FillValue");
                at->append_attr("_FillValue",var_type,fillvalue);               
            }
        }
        it++;
    }

}
#ifdef USE_HDFEOS2_LIB

bool HDFCFUtil::is_special_value(int32 dtype, float fillvalue, float realvalue) {

    bool ret_value = false;

    if (DFNT_UINT16 == dtype) {

        int fillvalue_int = (int)fillvalue;
        if (MAX_NON_SCALE_SPECIAL_VALUE == fillvalue_int) {
            int realvalue_int = (int)realvalue;
            if (realvalue_int <= MAX_NON_SCALE_SPECIAL_VALUE && realvalue_int >=MIN_NON_SCALE_SPECIAL_VALUE)
                ret_value = true;
        }
    }

    return ret_value;

}
int HDFCFUtil::check_geofile_dimmap(const string & geofilename) {

    int32 fileid = SWopen(const_cast<char*>(geofilename.c_str()),DFACC_READ);
    if (fileid < 0) 
        return -1;
    string swathname = "MODIS_Swath_Type_GEO";
    int32 datasetid = SWattach(fileid,const_cast<char*>(swathname.c_str()));
    if (datasetid < 0) {
        SWclose(fileid);
        return -1;
    }

    int32 nummaps = 0;
    int32 bufsize;

    // Obtain number of dimension maps and the buffer size.
    if ((nummaps = SWnentries(datasetid, HDFE_NENTMAP, &bufsize)) == -1) {
        SWdetach(datasetid);
        SWclose(fileid);
        return -1;
    }

    SWdetach(datasetid);
    SWclose(fileid);
    return nummaps;

}

// MODIS SCALE OFFSET HANDLING
// Note: MODIS Scale and offset handling needs to re-organized. But it may take big efforts.
// Instead, I remove the global variable mtype, and _das; move the old calculate_dtype code
// back to HDFEOS2.cc. The code is a little better organized. If possible, we may think to overhaul
// the handling of MODIS scale-offset part. KY 2012-6-19
// 
bool HDFCFUtil::change_data_type(DAS & das, SOType scaletype, string new_field_name) 
{
    AttrTable *at = das.get_table(new_field_name);

    if(scaletype!=DEFAULT_CF_EQU && at!=NULL)
    {
        AttrTable::Attr_iter it = at->attr_begin();
        string  scale_factor_value="";
        string  add_offset_value="0";
        string  radiance_scales_value="";
        string  radiance_offsets_value="";
        string  reflectance_scales_value=""; 
        string  reflectance_offsets_value="";
        string  scale_factor_type, add_offset_type;
        while (it!=at->attr_end())
        {
            if(at->get_name(it)=="radiance_scales")
                radiance_scales_value = *(at->get_attr_vector(it)->begin());
            if(at->get_name(it)=="radiance_offsets")
                radiance_offsets_value = *(at->get_attr_vector(it)->begin());
            if(at->get_name(it)=="reflectance_scales")
                reflectance_scales_value = *(at->get_attr_vector(it)->begin());
            if(at->get_name(it)=="reflectance_offsets")
                reflectance_offsets_value = *(at->get_attr_vector(it)->begin());

            // Ideally may just check if the attribute name is scale_factor.
            // But don't know if some products use attribut name like "scale_factor  "
            // So now just filter out the attribute scale_factor_err. KY 2012-09-20
            if(at->get_name(it).find("scale_factor")!=string::npos){
                string temp_attr_name = at->get_name(it);
                if (temp_attr_name != "scale_factor_err") {
                    scale_factor_value = *(at->get_attr_vector(it)->begin());
                    scale_factor_type = at->get_type(it);
                }
            }
            if(at->get_name(it).find("add_offset")!=string::npos)
            {
                string temp_attr_name = at->get_name(it);
                if (temp_attr_name !="add_offset_err") {
                    add_offset_value = *(at->get_attr_vector(it)->begin());
                    add_offset_type = at->get_type(it);
                }
            }
            it++;
        }

        if((radiance_scales_value.length()!=0 && radiance_offsets_value.length()!=0) || (reflectance_scales_value.length()!=0 && reflectance_offsets_value.length()!=0))
            return true;
		
        if(scale_factor_value.length()!=0) 
        {
            // using == to compare the double values is dangerous. May improve this when we redo this function.  KY 2012-6-14
            if(!(atof(scale_factor_value.c_str())==1 && atof(add_offset_value.c_str())==0)) 
                return true;
        }
    }

    return false;
}

// Put the dim. map info. here
void HDFCFUtil::obtain_dimmap_info(const string& filename,HDFEOS2::Dataset*dataset,  vector<struct dimmap_entry> & dimmaps, string & modis_geofilename, bool& geofile_has_dimmap) {


        HDFEOS2::SwathDataset *sw = static_cast<HDFEOS2::SwathDataset *>(dataset);
        const vector<HDFEOS2::SwathDataset::DimensionMap*>& origdimmaps = sw->getDimensionMaps();
//cerr<<"origdimmaps size is "<<origdimmaps.size() <<endl;
        vector<HDFEOS2::SwathDataset::DimensionMap*>::const_iterator it_dmap;
        struct dimmap_entry tempdimmap;

        // if having dimension maps, we need to retrieve the dimension map info.
        for(size_t i=0;i<origdimmaps.size();i++){
            tempdimmap.geodim = origdimmaps[i]->getGeoDimension();
            tempdimmap.datadim = origdimmaps[i]->getDataDimension();
            tempdimmap.offset = origdimmaps[i]->getOffset();
            tempdimmap.inc    = origdimmaps[i]->getIncrement();
            dimmaps.push_back(tempdimmap);
        }

        string check_modis_geofile_key ="H4.EnableCheckMODISGeoFile";
        bool check_geofile_key = false;
        check_geofile_key = HDFCFUtil::check_beskeys(check_modis_geofile_key);

        // Only when there is dimension map, we need to consider the additional MODIS geolocation files.
        // Will check if the check modis_geo_location file key is turned on.
        if((origdimmaps.size() != 0) && (true == check_geofile_key) ) {

            // Has to use C-style since basename and dirname are not C++ routines.
            char*tempcstr;
            tempcstr = new char [filename.size()+1];
            strncpy (tempcstr,filename.c_str(),filename.size());
            string basefilename = basename(tempcstr);
            string dirfilename = dirname(tempcstr);
            delete [] tempcstr;

            // If the current file is a MOD03 or a MYD03 file, we don't need to check the extra MODIS geolocation file at all.
            bool is_modis_geofile = false;
            if(basefilename.size() >5) {
                if((0 == basefilename.compare(0,5,"MOD03")) || (0 == basefilename.compare(0,5,"MYD03"))) 
                    is_modis_geofile = true;
            }

            // This part is implemented specifically for supporting MODIS dimension map data.
            // MODIS Aqua Swath dimension map geolocation file always starts with MYD03
            // MODIS Terra Swath dimension map geolocation file always starts with MOD03
            // We will check the first three characters to see if the dimension map geolocation file exists.
            // An example MODIS swath filename is MOD05_L2.A2008120.0000.005.2008121182723.hdf
            // An example MODIS geo-location file name is MOD03.A2008120.0000.005.2010003235220.hdf
            // Notice that the "A2008120.0000" in the middle of the name is the "Acquistion Date" of the data
            // So the geo-location file name should have exactly the same string. We will use this
            // string to identify if a MODIS geo-location file exists or not.
            // Note the string size is 14(.A2008120.0000).
            // More information of naming convention, check http://modis-atmos.gsfc.nasa.gov/products_filename.html
            // KY 2010-5-10


            // Obtain string "MYD" or "MOD"
            // Here we need to consider when MOD03 or MYD03 use the dimension map. 

            if ((false == is_modis_geofile) && (basefilename.size() >3)) {

                string fnameprefix = basefilename.substr(0,3);

                if(fnameprefix == "MYD" || fnameprefix =="MOD") {
                    size_t fnamemidpos = basefilename.find(".A");
                    if(fnamemidpos != string::npos) {
       	                string fnamemiddle = basefilename.substr(fnamemidpos,14);
                        if(fnamemiddle.size()==14) {
                            string geofnameprefix = fnameprefix+"03";

                            // geofnamefp will be something like "MOD03.A2008120.0000"
                            string geofnamefp = geofnameprefix + fnamemiddle;
                            DIR *dirp;
                            struct dirent* dirs;
    
                            dirp = opendir(dirfilename.c_str());
                            if (NULL == dirp) 
                                throw InternalErr(__FILE__,__LINE__,"opendir fails.");

                            while ((dirs = readdir(dirp))!= NULL){
                                if(strncmp(dirs->d_name,geofnamefp.c_str(),geofnamefp.size())==0){
                                    modis_geofilename = dirfilename + "/"+ dirs->d_name;
                                    int num_dimmap = HDFCFUtil::check_geofile_dimmap(modis_geofilename);
                                    if (num_dimmap < 0) 
                                        throw InternalErr(__FILE__,__LINE__,"this file is not a MODIS geolocation file.");
                                    geofile_has_dimmap = (num_dimmap >0)?true:false;
                                    closedir(dirp);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
}

// This is for the case that the separate MODIS geo-location file is used.
// Some geolocation names at the MODIS data file are not consistent with
// the names in the MODIS geo-location file. So need to correct them.
bool HDFCFUtil::is_modis_dimmap_nonll_field(string & fieldname) {

    bool modis_dimmap_nonll_field = false;
    vector<string> modis_dimmap_nonll_fieldlist; 

    modis_dimmap_nonll_fieldlist.push_back("Height");
    modis_dimmap_nonll_fieldlist.push_back("SensorZenith");
    modis_dimmap_nonll_fieldlist.push_back("SensorAzimuth");
    modis_dimmap_nonll_fieldlist.push_back("Range");
    modis_dimmap_nonll_fieldlist.push_back("SolarZenith");
    modis_dimmap_nonll_fieldlist.push_back("SolarAzimuth");
    modis_dimmap_nonll_fieldlist.push_back("Land/SeaMask");
    modis_dimmap_nonll_fieldlist.push_back("gflags");
    modis_dimmap_nonll_fieldlist.push_back("Solar_Zenith");
    modis_dimmap_nonll_fieldlist.push_back("Solar_Azimuth");
    modis_dimmap_nonll_fieldlist.push_back("Sensor_Azimuth");
    modis_dimmap_nonll_fieldlist.push_back("Sensor_Zenith");

    map<string,string>modis_field_to_geofile_field;
    map<string,string>::iterator itmap;
    modis_field_to_geofile_field["Solar_Zenith"] = "SolarZenith";
    modis_field_to_geofile_field["Solar_Azimuth"] = "SolarAzimuth";
    modis_field_to_geofile_field["Sensor_Zenith"] = "SensorZenith";
    modis_field_to_geofile_field["Solar_Azimuth"] = "SolarAzimuth";

    for (unsigned int i = 0; i <modis_dimmap_nonll_fieldlist.size(); i++) {

        if (fieldname == modis_dimmap_nonll_fieldlist[i]) {
            itmap = modis_field_to_geofile_field.find(fieldname);
            if (itmap !=modis_field_to_geofile_field.end())
                fieldname = itmap->second;
            modis_dimmap_nonll_field = true;
            break;
        }
    }

    return modis_dimmap_nonll_field;
}

void HDFCFUtil::handle_modis_special_attrs(AttrTable *at, const string newfname, SOType sotype,  bool gridname_change_valid_range, bool changedtype, bool & change_fvtype){

                string scale_factor_type, add_offset_type, fillvalue_type, valid_range_type;
                string  scale_factor_value=""; 
                float   orig_scale_value = 1;
                string  add_offset_value="0"; 
                float   orig_offset_value = 0;
                bool add_offset_found = false;

                string  fillvalue="";

                string  valid_range_value="";
                bool has_valid_range = false;
                float orig_valid_min = 0;
                float orig_valid_max = 0;

                string number_type_value="";
                string number_type_dap_type="";


                AttrTable::Attr_iter it = at->attr_begin();

 		while (it!=at->attr_end())
                {
                    if(at->get_name(it)=="scale_factor")
                    {
                        scale_factor_value = (*at->get_attr_vector(it)->begin());
                        orig_scale_value = atof(scale_factor_value.c_str());
                        scale_factor_type = at->get_type(it);
                    }
                    if(at->get_name(it)=="add_offset")
                    {
                        add_offset_value = (*at->get_attr_vector(it)->begin());
                        orig_offset_value = atof(add_offset_value.c_str());
                        add_offset_type = at->get_type(it);
                        add_offset_found = true;
                    }
                    if(at->get_name(it)=="_FillValue")
                    {
                        fillvalue =  (*at->get_attr_vector(it)->begin());
                        fillvalue_type = at->get_type(it);
                    }
                    if(at->get_name(it)=="valid_range")
                    {
                        vector<string> *avalue = at->get_attr_vector(it);
                        vector<string>::iterator ait = avalue->begin();
                        while(ait!=avalue->end())
                        {
                            valid_range_value += *ait;
                            ait++;	
                            if(ait!=avalue->end())
                                valid_range_value += ", ";
                        }
                        valid_range_type = at->get_type(it);
                        if (false == gridname_change_valid_range) {
                            orig_valid_min = (float)(atof((avalue->at(0)).c_str()));
                            orig_valid_max = (float)(atof((avalue->at(1)).c_str()));
                        }
                        has_valid_range = true;
                    }

                    if(true == changedtype && (at->get_name(it)=="Number_Type"))
                    {
                        number_type_value = (*at->get_attr_vector(it)->begin());
                        number_type_dap_type= at->get_type(it);
                    }

                    it++;
                }

                // Change scale and offset values
                if(scale_factor_value.length()!=0)
                {
                    if(!(atof(scale_factor_value.c_str())==1 && atof(add_offset_value.c_str())==0)) //Rename them.
                    {
                        at->del_attr("scale_factor");
                        at->append_attr("orig_scale_factor", scale_factor_type, scale_factor_value);
                        if(add_offset_found)
                        {
                            at->del_attr("add_offset");
                            at->append_attr("orig_add_offset", add_offset_type, add_offset_value);
                        }
                    }

                }
                if(true == changedtype && fillvalue.length()!=0 && fillvalue_type!="Float32" && fillvalue_type!="Float64") // Change attribute type.
                {
                    change_fvtype = true;
                    at->del_attr("_FillValue");
                    at->append_attr("_FillValue", "Float32", fillvalue);
                }

 
                float valid_max = 0;
                float valid_min = 0;

                it = at->attr_begin();
                bool handle_modis_l1b = false;
                if (sotype == MODIS_MUL_SCALE && true ==changedtype) {

                    // Emissive should be at the end of the field name such as "..._Emissive"
                    string emissive_str = "Emissive";
                    string RefSB_str = "RefSB";
                    bool is_emissive_field = false;
                    bool is_refsb_field = false;
                    if(newfname.find(emissive_str)!=string::npos) {
                        if(0 == newfname.compare(newfname.size()-emissive_str.size(),emissive_str.size(),emissive_str))
                            is_emissive_field = true;
                    }

                    if(newfname.find(RefSB_str)!=string::npos) {
                        if(0 == newfname.compare(newfname.size()-RefSB_str.size(),RefSB_str.size(),RefSB_str))
                            is_refsb_field = true;
                    }

                    if ((true == is_emissive_field) || (true== is_refsb_field)){

                        float scale_max = 0;
                        float scale_min = 100000;

                        float offset_max = 0;
                        float offset_min = 0;

                        float temp_var_val = 0;

                        string orig_long_name_value;
                        string modify_long_name_value;
                        string str_removed_from_long_name=" Scaled Integers";
                        string radiance_units_value;
                        
                        while (it!=at->attr_end()) 
		        {
                            // Radiance field(Emissive field)
                            if(true == is_emissive_field) {
                           
                                if ("radiance_scales" == (at->get_name(it))) {

                                    vector<string> *avalue = at->get_attr_vector(it);
                                    for (vector<string>::const_iterator ait = avalue->begin();ait !=avalue->end();++ait) {
                                        temp_var_val = (float)(atof((*ait).c_str())); 
                                        if (temp_var_val > scale_max) 
                                            scale_max = temp_var_val;
                                        if (temp_var_val < scale_min)
                                            scale_min = temp_var_val;
                                    }
                                }

                                if ("radiance_offsets" == (at->get_name(it))) {

                                    vector<string> *avalue = at->get_attr_vector(it);
                                    for (vector<string>::const_iterator ait = avalue->begin();ait !=avalue->end();++ait) {
                                        temp_var_val = (float)(atof((*ait).c_str())); 
                                        if (temp_var_val > offset_max) 
                                            offset_max = temp_var_val;
                                        if (temp_var_val < scale_min)
                                            offset_min = temp_var_val;
                                    }
                                }

                                if ("long_name" == (at->get_name(it))) {
                                    orig_long_name_value = (*at->get_attr_vector(it)->begin());
                                    if (orig_long_name_value.find(str_removed_from_long_name)!=string::npos) {
                                        if(0 == orig_long_name_value.compare(orig_long_name_value.size()-str_removed_from_long_name.size(), str_removed_from_long_name.size(),str_removed_from_long_name)) {

                                            modify_long_name_value = 
                                                orig_long_name_value.substr(0,orig_long_name_value.size()-str_removed_from_long_name.size()); 
                                            at->del_attr("long_name");
                                            at->append_attr("long_name","String",modify_long_name_value);
                                            at->append_attr("orig_long_name","String",orig_long_name_value);
                                        }
                                    }
                                }
                                if ("radiance_units" == (at->get_name(it))) 
                                    radiance_units_value = (*at->get_attr_vector(it)->begin());

                            }

                            if (true == is_refsb_field) {
                                if ("reflectance_scales" == (at->get_name(it))) {

                                    vector<string> *avalue = at->get_attr_vector(it);
                                    for (vector<string>::const_iterator ait = avalue->begin();ait !=avalue->end();++ait) {
                                        temp_var_val = (float)(atof((*ait).c_str())); 
                                        if (temp_var_val > scale_max) 
                                            scale_max = temp_var_val;
                                        if (temp_var_val < scale_min)
                                            scale_min = temp_var_val;
                                    }
                                }

                                if ("reflectance_offsets" == (at->get_name(it))) {

                                    vector<string> *avalue = at->get_attr_vector(it);
                                    for (vector<string>::const_iterator ait = avalue->begin();ait !=avalue->end();++ait) {
                                        temp_var_val = (float)(atof((*ait).c_str())); 
                                        if (temp_var_val > offset_max) 
                                            offset_max = temp_var_val;
                                        if (temp_var_val < scale_min)
                                            offset_min = temp_var_val;
                                    }
                                }

                                if ("long_name" == (at->get_name(it))) {
                                    orig_long_name_value = (*at->get_attr_vector(it)->begin());
                                    if (orig_long_name_value.find(str_removed_from_long_name)!=string::npos) {
                                        if(0 == orig_long_name_value.compare(orig_long_name_value.size()-str_removed_from_long_name.size(), str_removed_from_long_name.size(),str_removed_from_long_name)) {

                                            modify_long_name_value = 
                                                orig_long_name_value.substr(0,orig_long_name_value.size()-str_removed_from_long_name.size()); 
                                            at->del_attr("long_name");
                                            at->append_attr("long_name","String",modify_long_name_value);
                                            at->append_attr("orig_long_name","String",orig_long_name_value);
                                        }
                                    }
                                }
 
                            }
                            it++;
                        }
                        // Calculate the final valid_max and valid_min.
                        if (scale_min <= 0)
                            throw InternalErr(__FILE__,__LINE__,"the scale factor should always be greater than 0.");

                        if (orig_valid_max > offset_min) 
                            valid_max = (orig_valid_max-offset_min)*scale_max;
                        else 
                            valid_max = (orig_valid_max-offset_min)*scale_min;

                        if (orig_valid_min > offset_max)
                            valid_min = (orig_valid_min-offset_max)*scale_min;
                        else 
                            valid_min = (orig_valid_min -offset_max)*scale_max;

                        // These physical variables should be greater than 0
                        if (valid_min < 0)
                            valid_min = 0;
                        string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32,0,(void*)(&valid_min));
                        at->append_attr("valid_min","Float32",print_rep);
                        print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32,0,(void*)(&valid_max));
                        at->append_attr("valid_max","Float32",print_rep);

                        at->del_attr("valid_range");  

                        handle_modis_l1b = true;
                        
                        // Change the units for the emissive field
                        if (true == is_emissive_field && radiance_units_value.size() >0) {
                            at->del_attr("units");
                            at->append_attr("units","String",radiance_units_value);
                        }  
                    }
                }

                // Handle other valid_range attributes
                if(true == changedtype && true == has_valid_range && false == handle_modis_l1b) {
cerr<<"coming to generate valid_min and valid_max "<<endl;
                    if (true == gridname_change_valid_range) 
                        HDFCFUtil::handle_modis_vip_special_attrs(valid_range_value,scale_factor_value,valid_min,valid_max);


                    else if(scale_factor_value.length()!=0) {

                    if (MODIS_EQ_SCALE == sotype || MODIS_MUL_SCALE == sotype) {
                        if (orig_scale_value > 1) {
                            sotype = MODIS_DIV_SCALE;
                            (*BESLog::TheLog())<< "The field " << newfname << " scale factor is "<< orig_scale_value << endl
                               << " But the original scale factor type is MODIS_MUL_SCALE or MODIS_EQ_SCALE. " << endl
                               << " Now change it to MODIS_DIV_SCALE. "<<endl;
                        }
                    }

                    if (MODIS_DIV_SCALE == sotype) {
                        if (orig_scale_value < 1) {
                            sotype = MODIS_MUL_SCALE;
                            (*BESLog::TheLog())<< "The field " << newfname << " scale factor is "<< orig_scale_value << endl
                               << " But the original scale factor type is MODIS_DIV_SCALE. " << endl
                               << " Now change it to MODIS_MUL_SCALE. "<<endl;
                        }
                    }


                        if(sotype == MODIS_MUL_SCALE) {
                            valid_min = (orig_valid_min - orig_offset_value)*orig_scale_value;
                            valid_max = (orig_valid_max - orig_offset_value)*orig_scale_value;
                        }
                        else if (sotype == MODIS_DIV_SCALE) {
                            valid_min = (orig_valid_min - orig_offset_value)/orig_scale_value;
                            valid_max = (orig_valid_max - orig_offset_value)/orig_scale_value;
                        }
                        else if (sotype == MODIS_EQ_SCALE) {
                            valid_min = orig_valid_min * orig_scale_value + orig_offset_value;
                            valid_max = orig_valid_max * orig_scale_value + orig_offset_value;
                        }
                    }
                    else {
                        if (MODIS_MUL_SCALE == sotype || MODIS_DIV_SCALE == sotype) {
                            valid_min = orig_valid_min - orig_offset_value;
                            valid_max = orig_valid_max - orig_offset_value;
                        }
                        else if (MODIS_EQ_SCALE == sotype) {
                            valid_min = orig_valid_min + orig_offset_value;
                            valid_max = orig_valid_max + orig_offset_value;
                        }
                    }
                    


                    string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32,0,(void*)(&valid_min));
                    at->append_attr("valid_min","Float32",print_rep);
                    print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32,0,(void*)(&valid_max));
                    at->append_attr("valid_max","Float32",print_rep);

                    at->del_attr("valid_range");
                }

#if 0
                // Change scale and offset values
                if(scale_factor_value.length()!=0)
                {
                    if(!(atof(scale_factor_value.c_str())==1 && atof(add_offset_value.c_str())==0)) //Rename them.
                    {
                        at->del_attr("scale_factor");
                        at->append_attr("orig_scale_factor", scale_factor_type, scale_factor_value);
                        if(add_offset_found)
                        {
                            at->del_attr("add_offset");
                            at->append_attr("orig_add_offset", add_offset_type, add_offset_value);
                        }
                    }

                }
                if(true == changedtype && fillvalue.length()!=0 && fillvalue_type!="Float32" && fillvalue_type!="Float64") // Change attribute type.
                {
                    change_fvtype = true;
                    at->del_attr("_FillValue");
                    at->append_attr("_FillValue", "Float32", fillvalue);
                }
#endif

                // We also find that there is an attribute called "Number_Type" that will stores the original attribute
                // datatype. If the datatype gets changed, the attribute is confusion. here we can change the attribute
                // name to "Number_Type_Orig"
                if(true == changedtype && number_type_dap_type !="" ) {
                    at->del_attr("Number_Type");
                    at->append_attr("Number_Type_Orig",number_type_dap_type,number_type_value);

                }



}

void HDFCFUtil::handle_modis_vip_special_attrs(const std::string& valid_range_value, const std::string& scale_factor_value,float& valid_min, float &valid_max) {

                        int16 vip_orig_valid_min = 0;
                        int16 vip_orig_valid_max = 0;

                        size_t found = valid_range_value.find_first_of(",");
                        size_t found_from_end = valid_range_value.find_last_of(",");
                        if (string::npos == found) 
                            throw InternalErr(__FILE__,__LINE__,"should find the separator ,");
                        if (found != found_from_end)
                            throw InternalErr(__FILE__,__LINE__,"There should be only one separator.");
                        
                        //istringstream(valid_range_value.substr(0,found))>>orig_valid_min;
                        //istringstream(valid_range_value.substr(found+1))>>orig_valid_max;

                        vip_orig_valid_min = atoi((valid_range_value.substr(0,found)).c_str());
                        vip_orig_valid_max = atoi((valid_range_value.substr(found+1)).c_str());

                        int16 scale_factor_number = 1;
                        //istringstream(scale_factor_value)>>scale_factor_number;
                        scale_factor_number = atoi(scale_factor_value.c_str());

                        valid_min = (float)(vip_orig_valid_min/scale_factor_number);
                        valid_max = (float)(vip_orig_valid_max/scale_factor_number);
 

}

void HDFCFUtil::handle_amsr_attrs(AttrTable *at) {

                    AttrTable::Attr_iter it = at->attr_begin();
                string scale_factor_value="", add_offset_value="0";
                string scale_factor_type, add_offset_type;
                bool OFFSET_found = false;
                bool Scale_found = false;
                bool SCALE_FACTOR_found = false;
                while (it!=at->attr_end()) 
                {
                    if(at->get_name(it)=="SCALE_FACTOR")
                    {
                        scale_factor_value = (*at->get_attr_vector(it)->begin());
                        scale_factor_type = at->get_type(it);
                        SCALE_FACTOR_found = true;
                    }

                    if(at->get_name(it)=="Scale")
                    {
                        scale_factor_value = (*at->get_attr_vector(it)->begin());
                        scale_factor_type = at->get_type(it);
                        Scale_found = true;
                    }
                    if(at->get_name(it)=="OFFSET")
                    {
                        add_offset_value = (*at->get_attr_vector(it)->begin());
                        add_offset_type = at->get_type(it);
                        OFFSET_found = true;
                    }
                    it++;
                }

                if (true == SCALE_FACTOR_found) {
                    at->del_attr("SCALE_FACTOR");
                    at->append_attr("scale_factor",scale_factor_type,scale_factor_value);
                }

                if (true == Scale_found) {
                    at->del_attr("Scale");
                    at->append_attr("scale_factor",scale_factor_type,scale_factor_value);
                }


                if (true == OFFSET_found) {
                    at->del_attr("OFFSET");
                    at->append_attr("add_offset",add_offset_type,add_offset_value);
                }

}



#endif

void HDFCFUtil::check_obpg_global_attrs(HDFSP::File *f, std::string & scaling, float & slope,bool &global_slope_flag,float & intercept, bool & global_intercept_flag){

    HDFSP::SD* spsd = f->getSD();

    for(vector<HDFSP::Attribute *>::const_iterator i=spsd->getAttributes().begin();i!=spsd->getAttributes().end();i++) {
       
        //We want to add two new attributes, "scale_factor" and "add_offset" to data fields if the scaling equation is linear. 
        // OBPG products use "Slope" instead of "scale_factor", "intercept" instead of "add_offset". "Scaling" describes if the equation is linear.
        // Their values will be copied directly from File attributes. This addition is for OBPG L3 only.
        // We also add OBPG L2 support since all OBPG level 2 products(CZCS,MODISA,MODIST,OCTS,SeaWiFS) we evaluate use Slope,intercept and linear equation
        // for the final data. KY 2012-09-06
	if(f->getSPType()==OBPGL3 || f->getSPType() == OBPGL2)
        {
            if((*i)->getName()=="Scaling")
            {
                string tmpstring((*i)->getValue().begin(), (*i)->getValue().end());
                scaling = tmpstring;
            }
            if((*i)->getName()=="Slope" || (*i)->getName()=="slope")
            {
                global_slope_flag = true;
			
                switch((*i)->getType())
                {
#define GET_SLOPE(TYPE, CAST) \
    case DFNT_##TYPE: \
    { \
        CAST tmpvalue = *(CAST*)&((*i)->getValue()[0]); \
        slope = (float)tmpvalue; \
    } \
    break;

                    GET_SLOPE(INT16,   int16);
                    GET_SLOPE(INT32,   int32);
                    GET_SLOPE(FLOAT32, float);
                    GET_SLOPE(FLOAT64, double);
#undef GET_SLOPE
                } ;
                //slope = *(float*)&((*i)->getValue()[0]);
            }
            if((*i)->getName()=="Intercept" || (*i)->getName()=="intercept")
            {	
                global_intercept_flag = true;
                switch((*i)->getType())
                {
#define GET_INTERCEPT(TYPE, CAST) \
    case DFNT_##TYPE: \
    { \
        CAST tmpvalue = *(CAST*)&((*i)->getValue()[0]); \
        intercept = (float)tmpvalue; \
    } \
    break;
                    GET_INTERCEPT(INT16,   int16);
                    GET_INTERCEPT(INT32,   int32);
                    GET_INTERCEPT(FLOAT32, float);
                    GET_INTERCEPT(FLOAT64, double);
#undef GET_INTERCEPT
                } ;
                //intercept = *(float*)&((*i)->getValue()[0]);
            }
        }
    }
}

void HDFCFUtil::add_obpg_special_attrs(HDFSP::File*f,DAS &das,HDFSP::SDField *onespsds, string& scaling, float& slope, bool& global_slope_flag, float& intercept,bool & global_intercept_flag) {

    // The following code checks the special handling of scale and offset of the OBPG products. 


//    vector<HDFSP::SDField *>::const_iterator it_g;
 //   for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){

        // Ignore ALL coordinate variables if this is "OTHERHDF" case and some dimensions 
        // don't have dimension scale data.
    
        AttrTable *at = das.get_table(onespsds->getNewName());
        if (!at)
            at = das.add_table(onespsds->getNewName(), new AttrTable);


        //For OBPG L2 and L3 only.Some OBPG products put "slope" and "Intercept" etc. in the field attributes
        // We still need to handle the scale and offset here.
        bool scale_factor_flag = false;
        bool add_offset_flag = false;
        bool slope_flag = false;
        bool intercept_flag = false;

        if(f->getSPType()==OBPGL3 || f->getSPType() == OBPGL2) {// Begin OPBG CF attribute handling(Checking "slope" and "intercept") 
            for(vector<HDFSP::Attribute *>::const_iterator i=onespsds->getAttributes().begin();i!=onespsds->getAttributes().end();i++) {
                if(global_slope_flag != true && ((*i)->getName()=="Slope" || (*i)->getName()=="slope"))
                {
                    slope_flag = true;
			
                    switch((*i)->getType())
                    {
#define GET_SLOPE(TYPE, CAST) \
    case DFNT_##TYPE: \
    { \
        CAST tmpvalue = *(CAST*)&((*i)->getValue()[0]); \
        slope = (float)tmpvalue; \
    } \
    break;

                        GET_SLOPE(INT16,   int16);
                        GET_SLOPE(INT32,   int32);
                        GET_SLOPE(FLOAT32, float);
                        GET_SLOPE(FLOAT64, double);
#undef GET_SLOPE
                    } ;
                    //slope = *(float*)&((*i)->getValue()[0]);
                }
                if(global_intercept_flag != true && ((*i)->getName()=="Intercept" || (*i)->getName()=="intercept"))
                {	
                    intercept_flag = true;
                    switch((*i)->getType())
                    {
#define GET_INTERCEPT(TYPE, CAST) \
    case DFNT_##TYPE: \
    { \
        CAST tmpvalue = *(CAST*)&((*i)->getValue()[0]); \
        intercept = (float)tmpvalue; \
    } \
    break;
                        GET_INTERCEPT(INT16,   int16);
                        GET_INTERCEPT(INT32,   int32);
                        GET_INTERCEPT(FLOAT32, float);
                        GET_INTERCEPT(FLOAT64, double);
#undef GET_INTERCEPT
                    } ;
                    //intercept = *(float*)&((*i)->getValue()[0]);
                }
            }
        } // End of checking "slope" and "intercept"

        // Checking if OBPG has "scale_factor" ,"add_offset", generally checking for "long_name" attributes.
        for(vector<HDFSP::Attribute *>::const_iterator i=onespsds->getAttributes().begin();i!=onespsds->getAttributes().end();i++) {       

            if((f->getSPType()==OBPGL3 || f->getSPType() == OBPGL2)  && (*i)->getName()=="scale_factor")
                scale_factor_flag = true;		

            if((f->getSPType()==OBPGL3 || f->getSPType() == OBPGL2) && (*i)->getName()=="add_offset")
                add_offset_flag = true;
        }
        
        // Checking if the scale and offset equation is linear, this is only for OBPG level 3.
        // Also checking if having the fill value and adding fill value if not having one for some OBPG data type
        if((f->getSPType() == OBPGL2 || f->getSPType()==OBPGL3 )&& onespsds->getFieldType()==0)
        {
            
            if((OBPGL3 == f->getSPType() && (scaling.find("linear")!=string::npos)) || OBPGL2 == f->getSPType() ) {

                if(false == scale_factor_flag && (true == slope_flag || true == global_slope_flag))
                {
                    string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32, 0, (void*)&(slope));
                    at->append_attr("scale_factor", HDFCFUtil::print_type(DFNT_FLOAT32), print_rep);
                }

                if(false == add_offset_flag && (true == intercept_flag || true == global_intercept_flag))
                {
                    string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32, 0, (void*)&(intercept));
                    at->append_attr("add_offset", HDFCFUtil::print_type(DFNT_FLOAT32), print_rep);
                }
            }

            bool has_fill_value = false;
            for(vector<HDFSP::Attribute *>::const_iterator i=onespsds->getAttributes().begin();i!=onespsds->getAttributes().end();i++) {
                if ("_FillValue" == (*i)->getNewName()){
                    has_fill_value = true; 
                    break;
                }
            }
         

            // Add fill value to some type of OBPG data. 16-bit integer, fill value = -32767, unsigned 16-bit integer fill value = 65535
            // This is based on the evaluation of the example files. KY 2012-09-06
            if ((false == has_fill_value) &&(DFNT_INT16 == onespsds->getType())) {
                short fill_value = -32767;
                string print_rep = HDFCFUtil::print_attr(DFNT_INT16,0,(void*)&(fill_value));
                at->append_attr("_FillValue",HDFCFUtil::print_type(DFNT_INT16),print_rep);
            }

            if ((false == has_fill_value) &&(DFNT_UINT16 == onespsds->getType())) {
                unsigned short fill_value = 65535;
                string print_rep = HDFCFUtil::print_attr(DFNT_UINT16,0,(void*)&(fill_value));
                at->append_attr("_FillValue",HDFCFUtil::print_type(DFNT_UINT16),print_rep);
            }

        }// Finish OBPG handling

}

void HDFCFUtil::handle_otherhdf_special_attrs(HDFSP::File*f,DAS &das) {

    // For some HDF4 files that follow HDF4 dimension scales, P.O. DAAC's AVHRR files.
    // The "otherHDF" category can almost make AVHRR files work, except
    // that AVHRR uses the attribute name "unit" instead of "units" for latitude and longitude,
    // I have to correct the name to follow CF conventions(using "units"). I won't check
    // the latitude and longitude values since latitude and longitude values for some files(LISO files)   
    // are not in the standard range(0-360 for lon and 0-180 for lat). KY 2011-3-3

    const vector<HDFSP::SDField *>& spsds = f->getSD()->getFields();
    vector<HDFSP::SDField *>::const_iterator it_g;

    if(f->getSPType() == OTHERHDF) {

        bool latflag = false;
        bool latunitsflag = false; //CF conventions use "units" instead of "unit"
        bool lonflag = false;
        bool lonunitsflag = false; // CF conventions use "units" instead of "unit"
        int llcheckoverflag = 0;

        // Here I try to condense the code within just two for loops
        // The outer loop: Loop through all SDS objects
        // The inner loop: Loop through all attributes of this SDS
        // Inside two inner loops(since "units" are common for other fields), 
        // inner loop 1: when an attribute's long name value is L(l)atitude,mark it.
        // inner loop 2: when an attribute's name is units, mark it.
        // Outside the inner loop, when latflag is true, and latunitsflag is false,
        // adding a new attribute called units and the value should be "degrees_north".
        // doing the same thing for longitude.

        for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){

            // Ignore ALL coordinate variables if this is "OTHERHDF" case and some dimensions 
            // don't have dimension scale data.
            if ( true == f->Has_Dim_NoScale_Field() && ((*it_g)->getFieldType() !=0) && ((*it_g)->IsDimScale() == false))
                continue;

            // Ignore the empty(no data) dimension variable.
            if (OTHERHDF == f->getSPType() && true == (*it_g)->IsDimNoScale())
                continue;

            AttrTable *at = das.get_table((*it_g)->getNewName());
            if (!at)
                at = das.add_table((*it_g)->getNewName(), new AttrTable);

            for(vector<HDFSP::Attribute *>::const_iterator i=(*it_g)->getAttributes().begin();i!=(*it_g)->getAttributes().end();i++) {
                if((*i)->getType()==DFNT_UCHAR || (*i)->getType() == DFNT_CHAR){

                    if((*i)->getName() == "long_name") {
                        string tempstring2((*i)->getValue().begin(),(*i)->getValue().end());
                        string tempfinalstr= string(tempstring2.c_str());// This may remove some garbage characters
                        if(tempfinalstr=="latitude" || tempfinalstr == "Latitude") // Find long_name latitude
                            latflag = true;
                        if(tempfinalstr=="longitude" || tempfinalstr == "Longitude") // Find long_name latitude
                            lonflag = true;
                    }
                }
            }

            if(latflag) {
                for(vector<HDFSP::Attribute *>::const_iterator i=(*it_g)->getAttributes().begin();i!=(*it_g)->getAttributes().end();i++) {
                    if((*i)->getName() == "units") 
                        latunitsflag = true;
                }
            }

            if(lonflag) {
                for(vector<HDFSP::Attribute *>::const_iterator i=(*it_g)->getAttributes().begin();i!=(*it_g)->getAttributes().end();i++) {
                    if((*i)->getName() == "units") 
                        lonunitsflag = true;
                }
            }
            if(latflag && !latunitsflag){ // No "units" for latitude, add "units"
                at->append_attr("units","String","degrees_north");
                latflag = false;
                latunitsflag = false;
                llcheckoverflag++;
            }

            if(lonflag && !lonunitsflag){ // No "units" for latitude, add "units"
                at->append_attr("units","String","degrees_east");
                lonflag = false;
                latunitsflag = false;
                llcheckoverflag++;
            }
            if(llcheckoverflag ==2) break;

        }

    }

}

    //
    // Many CERES products compose of multiple groups
    // There are many fields in CERES data(a few hundred) and the full name(with the additional path)
    // is very long. It causes Java clients choken since Java clients append names in the URL
    // To improve the performance and to make Java clients access the data, we will ignore 
    // the path of these fields. Users can turn off this feature by commenting out the line: H4.EnableCERESMERRAShortName=true
    // or can set the H4.EnableCERESMERRAShortName=false
    // We still preserve the full path as an attribute in case users need to check them. 
    // Kent 2012-6-29
 
void HDFCFUtil::handle_merra_ceres_attrs_with_bes_keys(HDFSP::File *f, DAS &das,string filename) {


    string check_ceres_merra_short_name_key="H4.EnableCERESMERRAShortName";
    bool turn_on_ceres_merra_short_name_key= false;
    string base_filename = filename.substr(filename.find_last_of("/")+1);

    turn_on_ceres_merra_short_name_key = HDFCFUtil::check_beskeys(check_ceres_merra_short_name_key);

    if (true == turn_on_ceres_merra_short_name_key && (CER_ES4 == f->getSPType() || CER_SRB == f->getSPType()
        || CER_CDAY == f->getSPType() || CER_CGEO == f->getSPType() 
        || CER_SYN == f->getSPType() || CER_ZAVG == f->getSPType()
        || CER_AVG == f->getSPType() || (0== (base_filename.compare(0,5,"MERRA"))))) {

        const vector<HDFSP::SDField *>& spsds = f->getSD()->getFields();
        vector<HDFSP::SDField *>::const_iterator it_g;
        for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){

            AttrTable *at = das.get_table((*it_g)->getNewName());
            if (!at)
                at = das.add_table((*it_g)->getNewName(), new AttrTable);

            at->append_attr("fullpath","String",(*it_g)->getSpecFullPath());

        }

    }

}


void HDFCFUtil::handle_vdata_attrs_with_desc_key(HDFSP::File*f,libdap::DAS &das) {

    // Check the EnableVdataDescAttr key. If this key is turned on, the handler-added attribute VDdescname and
    // the attributes of vdata and vdata fields will be outputed to DAS. Otherwise, these attributes will
    // not outputed to DAS. The key will be turned off by default to shorten the DAP output. KY 2012-09-18

    string check_vdata_desc_key="H4.EnableVdataDescAttr";
    bool turn_on_vdata_desc_key= false;

    turn_on_vdata_desc_key = HDFCFUtil::check_beskeys(check_vdata_desc_key);

    string VDdescname = "hdf4_vd_desc";
    string VDdescvalue = "This is an HDF4 Vdata.";
    string VDfieldprefix = "Vdata_field_";
    string VDattrprefix = "Vdata_attr_";
    string VDfieldattrprefix ="Vdata_field_attr_";


    if(f->getSPType() != CER_AVG && f->getSPType() != CER_ES4 && f->getSPType() !=CER_SRB && f->getSPType() != CER_ZAVG) {

        for(vector<HDFSP::VDATA *>::const_iterator i=f->getVDATAs().begin(); i!=f->getVDATAs().end();i++) {

            AttrTable *at = das.get_table((*i)->getNewName());
            if(!at)
                at = das.add_table((*i)->getNewName(),new AttrTable);
 
            if (true == turn_on_vdata_desc_key) {
                // Add special vdata attributes
                bool emptyvddasflag = true;
                if(!((*i)->getAttributes().empty())) emptyvddasflag = false;
                if(((*i)->getTreatAsAttrFlag()))
                    emptyvddasflag = false;
                else {
                    for(vector<HDFSP::VDField *>::const_iterator j=(*i)->getFields().begin();j!=(*i)->getFields().end();j++) {
                        if(!((*j)->getAttributes().empty())) {
                            emptyvddasflag = false;
                            break;
                        }
                    }
                }

                if(emptyvddasflag) 
                    continue;
                at->append_attr(VDdescname, "String" , VDdescvalue);

                for(vector<HDFSP::Attribute *>::const_iterator it_va = (*i)->getAttributes().begin();it_va!=(*i)->getAttributes().end();it_va++) {

                    if((*it_va)->getType()==DFNT_UCHAR || (*it_va)->getType() == DFNT_CHAR){

                        string tempstring2((*it_va)->getValue().begin(),(*it_va)->getValue().end());
                        string tempfinalstr= string(tempstring2.c_str());
                        at->append_attr(VDattrprefix+(*it_va)->getNewName(), "String" , tempfinalstr);
                    }
                    else {
                        for (int loc=0; loc < (*it_va)->getCount() ; loc++) {
                            string print_rep = HDFCFUtil::print_attr((*it_va)->getType(), loc, (void*) &((*it_va)->getValue()[0]));
                            at->append_attr(VDattrprefix+(*it_va)->getNewName(), HDFCFUtil::print_type((*it_va)->getType()), print_rep);
                        }
                    }

                }
            }

            if(!((*i)->getTreatAsAttrFlag())){ 

                if (true == turn_on_vdata_desc_key) {

                    //NOTE: for vdata field, we assume that no special characters are found 
                    for(vector<HDFSP::VDField *>::const_iterator j=(*i)->getFields().begin();j!=(*i)->getFields().end();j++) {

                        // This vdata field will NOT be treated as attributes, only save the field attribute as the attribute
                        for(vector<HDFSP::Attribute *>::const_iterator it_va = (*j)->getAttributes().begin();it_va!=(*j)->getAttributes().end();it_va++) {

                            if((*it_va)->getType()==DFNT_UCHAR || (*it_va)->getType() == DFNT_CHAR){

                                string tempstring2((*it_va)->getValue().begin(),(*it_va)->getValue().end());
                                string tempfinalstr= string(tempstring2.c_str());
                                at->append_attr(VDfieldattrprefix+(*j)->getNewName()+"_"+(*it_va)->getNewName(), "String" , tempfinalstr);
                            }
                            else {
                                for (int loc=0; loc < (*it_va)->getCount() ; loc++) {
                                    string print_rep = HDFCFUtil::print_attr((*it_va)->getType(), loc, (void*) &((*it_va)->getValue()[0]));
                                    at->append_attr(VDfieldattrprefix+(*j)->getNewName()+"_"+(*it_va)->getNewName(), HDFCFUtil::print_type((*it_va)->getType()), print_rep);
                                }
                            }

                        }
                    }
                }

            }

            else {

                for(vector<HDFSP::VDField *>::const_iterator j=(*i)->getFields().begin();j!=(*i)->getFields().end();j++) {
           
 
                    if((*j)->getFieldOrder() == 1) {
                        if((*j)->getType()==DFNT_UCHAR || (*j)->getType() == DFNT_CHAR){
                            string tempstring2((*j)->getValue().begin(),(*j)->getValue().end());
                            string tempfinalstr= string(tempstring2.c_str());
                            at->append_attr(VDfieldprefix+(*j)->getNewName(), "String" , tempfinalstr);
                        }
                        else {
                            for ( int loc=0; loc < (*j)->getNumRec(); loc++) {
                                string print_rep = HDFCFUtil::print_attr((*j)->getType(), loc, (void*) &((*j)->getValue()[0]));
                                at->append_attr(VDfieldprefix+(*j)->getNewName(), HDFCFUtil::print_type((*j)->getType()), print_rep);
                            }
                        }
                    }
                    else {//When field order is greater than 1,we want to print each record in group with single quote,'0 1 2','3 4 5', etc.

                        if((*j)->getValue().size() != (unsigned int)(DFKNTsize((*j)->getType())*((*j)->getFieldOrder())*((*j)->getNumRec()))){
                            throw InternalErr(__FILE__,__LINE__,"the vdata field size doesn't match the vector value");
                        }

                        if((*j)->getNumRec()==1){
                            if((*j)->getType()==DFNT_UCHAR || (*j)->getType() == DFNT_CHAR){
                                string tempstring2((*j)->getValue().begin(),(*j)->getValue().end());
                                string tempfinalstr= string(tempstring2.c_str());
                                at->append_attr(VDfieldprefix+(*j)->getNewName(),"String",tempfinalstr);
                            }
                            else {
                                for (int loc=0; loc < (*j)->getFieldOrder(); loc++) {
                                    string print_rep = HDFCFUtil::print_attr((*j)->getType(), loc, (void*) &((*j)->getValue()[0]));
                                    at->append_attr(VDfieldprefix+(*j)->getNewName(), HDFCFUtil::print_type((*j)->getType()), print_rep);
                                }
                            }

                        }

                        else {
                            if((*j)->getType()==DFNT_UCHAR || (*j)->getType() == DFNT_CHAR){

                                for(int tempcount = 0; tempcount < (*j)->getNumRec()*DFKNTsize((*j)->getType());tempcount ++) {
                                    vector<char>::const_iterator tempit;
                                    tempit = (*j)->getValue().begin()+tempcount*((*j)->getFieldOrder());
                                    string tempstring2(tempit,tempit+(*j)->getFieldOrder());
                                    string tempfinalstr= string(tempstring2.c_str());
                                    string tempoutstring = "'"+tempfinalstr+"'";
                                    at->append_attr(VDfieldprefix+(*j)->getNewName(),"String",tempoutstring);
                                }
                            }

                            else {
                                for(int tempcount = 0; tempcount < (*j)->getNumRec();tempcount ++) {
                                    at->append_attr(VDfieldprefix+(*j)->getNewName(),HDFCFUtil::print_type((*j)->getType()),"'");
                                    for (int loc=0; loc < (*j)->getFieldOrder(); loc++) {
                                        string print_rep = HDFCFUtil::print_attr((*j)->getType(), loc, (void*) &((*j)->getValue()[tempcount*((*j)->getFieldOrder())]));
                                        at->append_attr(VDfieldprefix+(*j)->getNewName(), HDFCFUtil::print_type((*j)->getType()), print_rep);
                                    }
                                    at->append_attr(VDfieldprefix+(*j)->getNewName(),HDFCFUtil::print_type((*j)->getType()),"'");
                                }
                            }
                        }
                    }

         
                    if (true == turn_on_vdata_desc_key) {
                        for(vector<HDFSP::Attribute *>::const_iterator it_va = (*j)->getAttributes().begin();it_va!=(*j)->getAttributes().end();it_va++) {

                            if((*it_va)->getType()==DFNT_UCHAR || (*it_va)->getType() == DFNT_CHAR){

                                string tempstring2((*it_va)->getValue().begin(),(*it_va)->getValue().end());
                                string tempfinalstr= string(tempstring2.c_str());
                                at->append_attr(VDfieldattrprefix+(*it_va)->getNewName(), "String" , tempfinalstr);
                            }
                            else {
                                for (int loc=0; loc < (*it_va)->getCount() ; loc++) {
                                    string print_rep = HDFCFUtil::print_attr((*it_va)->getType(), loc, (void*) &((*it_va)->getValue()[0]));
                                    at->append_attr(VDfieldattrprefix+(*it_va)->getNewName(), HDFCFUtil::print_type((*it_va)->getType()), print_rep);
                                }
                            }
                        }
                    }
                }
            }

        }
    } 

}



