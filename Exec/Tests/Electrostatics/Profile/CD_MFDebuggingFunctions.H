#ifndef __CD_MFDebuggingFunctions__
#define __CD_MFDebuggingFunctions__

/**
   Purpose:
   Print data (very) neatly to screen.   Makes clear what data is covered.
   This stuff is meant to be called from gdb.
   Example (say you have an EBCellFAB called phi):
   gdb> p MFDebug::dumpEBCellArea(&phi)
   You can also call any of this from source, of course.
   Send NULL to any of the functions to do nothing.

   This is the horrible way to set the area.
   Previous APIs were worse.
**/
#define AREA_LO_X 0  
#define AREA_LO_Y 0  
#define AREA_LO_Z 0  
#define AREA_HI_X 7  
#define AREA_HI_Y 7  
#define AREA_HI_Z 7  
namespace MFDebug
{

  ///
  CH_XD::Box
  getAreaBox()
  {
    typedef CH_XD::IntVect   ch_iv;
    typedef CH_XD::Box       ch_box;
    ch_iv  areaLo(D_DECL(AREA_LO_X, AREA_LO_Y, AREA_LO_Z));
    ch_iv  areaHi(D_DECL(AREA_HI_X, AREA_HI_Y, AREA_HI_Z));
    ch_box areaBox(areaLo, areaHi);
    return areaBox;
  }

  ///
  void 
  dumpEBCellArea(CH_XD::EBCellFAB* a_dataPtr)
  {
    typedef CH_XD::IntVect   ch_iv;
    typedef CH_XD::Box       ch_box;
    typedef CH_XD::EBGraph   ch_ebg;
    typedef CH_XD::EBCellFAB ch_ebcf;
  
    if(a_dataPtr != NULL)
    {
      ch_box areaBox = getAreaBox();
      cout    << setprecision(8)
              << setiosflags(ios::showpoint)
              << setiosflags(ios::scientific);

      ch_ebg graph = a_dataPtr->getEBISBox().getEBGraph();
      ch_box area  = areaBox;
      area &= graph.getDomain();

      ch_ebcf& data = *a_dataPtr;
      ch_box   databox  = a_dataPtr->box();
      ch_box   printBox = databox & areaBox;
      ch_iv    hideb    = printBox.bigEnd();
      ch_iv    lodeb    = printBox.smallEnd();
      for(int ivar = 0; ivar < data.nComp(); ivar++)
      {
        cout << "data region contains, for variable "<< ivar << endl;
        for(int j = hideb[1]; j >= lodeb[1]; j--)
        {
          for(int i = lodeb[0]; i <= hideb[0]; i++)
          {
            IntVect pt(i,j);
            auto vofs = graph.getVoFs(pt);
            cout << pt << ":";
            if(vofs.size() >  0)
            {
              if(data(vofs[0], ivar)  >= 0)
              {
                cout << " ";
              }// 
              cout  << data(vofs[0], ivar) << "  ";
            } //end  if there is data
            else
            {
              cout  << "---------------" << "  ";
            }
          }
          cout << endl;
        }
      }
    }
  } //end function dumpEBCellArea

  void 
  dumpMFCellArea(CH_XD::MFCellFAB* a_dataPtr)
  {
    if(a_dataPtr != NULL)
    {
      auto& phase_vec = a_dataPtr->m_phase;
      for(int iphase = 0; iphase < phase_vec.size(); iphase++)
      {
        cout << "dumpMFCellArea: dumping phase << " << iphase << endl;
        dumpEBCellArea(phase_vec[iphase]);
      }
    }
  }


  ////
  void 
  dumpLDMFCellArea(CH_XD::LevelData<CH_XD::MFCellFAB>* a_dataPtr)
  {
    if(a_dataPtr != NULL)
    {
      typedef CH_XD::IntVect   ch_iv;
      typedef CH_XD::Box       ch_box;
      typedef CH_XD::EBGraph   ch_ebg;
      typedef CH_XD::EBCellFAB ch_ebcf;
      typedef CH_XD::MFCellFAB ch_mfcf;
      
      auto dit = a_dataPtr->dataIterator();
      auto dbl = a_dataPtr->disjointBoxLayout();
      ch_box areaBox = getAreaBox();
      cout << "dumpLDMFCellArea: areaBox = "<< areaBox<< endl;
      for(int ibox = 0; ibox < dit.size(); ibox++)
      {
        ch_box valid = dbl[dit[ibox]];
        ch_box inter = valid & areaBox;
        if(inter.ok())
        {
          cout << "begin ibox = " << ibox ;
          cout << ", valid = " << valid;
          cout << ", intersection box = " << inter;
          cout << ", data:  " << inter;

          auto& data = *a_dataPtr;
          dumpMFCellArea(&data[dit[ibox]]);
          cout << "end ibox = " << ibox << endl;
        }
        else
        {
          cout << "valid box " << valid << " has no intersection with area box " << areaBox << endl;
        }
      }
    }
  }// end function dmpMFCellArea

  ///now aint that a mouthfull
  void 
  dumpMFAMRCellArea(ChomboDischarge::EBAMRData<CH_XD::MFCellFAB>* a_dataPtr)
  {
    if(a_dataPtr != NULL)
    {
      int nlevels = a_dataPtr->size();
      cout << "dumpVRCPLDMFCellArea::vector size = " << nlevels << endl;;
      auto& ebamrdata = *a_dataPtr;
      for(int ilev = 0; ilev < nlevels; ilev++)
      {
        /**
           This is a RefCountedPtr<LevelData < MFCellFAB > > doo dah, doo dah.
        **/
        auto& level_rcp  = ebamrdata[ilev];
        auto& level_data = *(level_rcp);
        cout << "begin level = " << ilev << endl;
        dumpLDMFCellArea(&level_data);
        cout << "end   level = " << ilev << endl;
      }
        
    }
  }
  ///old Vince Beckner trick. 
  inline void func_to_get_functions_into_namespace()
  {
    dumpMFAMRCellArea(NULL);
    dumpEBCellArea(NULL);
    dumpMFCellArea(NULL);
    dumpLDMFCellArea(NULL);
  }

} //namespace MFDebug

///avoid side effects
#undef AREA_LO_X 
#undef AREA_LO_Y 
#undef AREA_LO_Z 
#undef AREA_HI_X 
#undef AREA_HI_Y 
#undef AREA_HI_Z 
///include guard
#endif
