/*!
 ***************************************************************************
 * \file md_high.c
 *
 * \brief
 *    Main macroblock mode decision functions and helpers
 *
 **************************************************************************
 */

#include <math.h>
#include <limits.h>
#include <float.h>

#include "global.h"
#include "rdopt_coding_state.h"
#include "intrarefresh.h"
#include "image.h"
#include "ratectl.h"
#include "mode_decision.h"
#include "mode_decision_p8x8.h"
#include "fmo.h"
#include "me_umhex.h"
#include "me_umhexsmp.h"
#include "macroblock.h"
#include "md_common.h"
#include "conformance.h"
#include "vlc.h"
#include "rdopt.h"
#include "mv_search.h"

/*!
*************************************************************************************
* \brief
*    Mode Decision for a macroblock
*************************************************************************************
*/

/*
  各个Lambda的含义如下（定义在RD_PARAMS：enc_mb）中
  double lambda_md;        //!< Mode decision Lambda
  int    lambda_mdfp;       //!< Fixed point mode decision lambda;
  double lambda_me[3];     //!< Motion Estimation Lambda
  int    lambda_mf[3];     //!< Integer formatted Motion Estimation Lambda
*/

/*
  共七种模式：
  0:16X16 Direct模式,在B帧中有效                                                                                                                                  
  1:Inter16X16，在帧间有效                                 
  2:Inter16X8,在帧间有效                                     
  3:Inter8X16,在帧间有效                                             
  P8X8:帧间有效:包括Inter8x8,Inter8x4,Inter4x8,Inter4x4                                                 
  I16MB:Intra16X16帧内有效                                                         
  I4MB:Intra有效                                                     
  I8MB:Intra有效                                                         
  IPCM:Intra有效，不要预测，直接对RAW数据编码.                       
*/

void encode_one_macroblock_high (Macroblock *currMB)
{
  Slice *currSlice = currMB->p_Slice;
  VideoParameters *p_Vid = currMB->p_Vid;
  InputParameters *p_Inp = currMB->p_Inp;
  PicMotionParams **motion = p_Vid->enc_picture->mv_info;
  RDOPTStructure  *p_RDO = currSlice->p_RDO;

  int         max_index = 9;
  int         block, index, mode, i, j;
  RD_PARAMS   enc_mb;
  distblk     bmcost[5] = {DISTBLK_MAX};
  distblk     cost=0;
  distblk     min_cost = DISTBLK_MAX;
  int         intra1 = 0;
  int         mb_available[3];

  short       bslice      = (short) (currSlice->slice_type == B_SLICE);
  short       pslice      = (short) ((currSlice->slice_type == P_SLICE) || (currSlice->slice_type == SP_SLICE));
  short       intra       = (short) ((currSlice->slice_type == I_SLICE) || (currSlice->slice_type == SI_SLICE) || (pslice && currMB->mb_y == p_Vid->mb_y_upd && p_Vid->mb_y_upd != p_Vid->mb_y_intra));
  int         lambda_mf[3];

  imgpel    **mb_pred  = currSlice->mb_pred[0];
  Block8x8Info *b8x8info = p_Vid->b8x8info;

  char        chroma_pred_mode_range[2];
  short       inter_skip = 0;
  BestMode    md_best;
  Info8x8     best;


  init_md_best(&md_best);

  // Init best (need to create simple function)
  best.pdir = 0;
  best.bipred = 0;
  best.ref[LIST_0] = 0;
  best.ref[LIST_1] = -1;

  intra |= RandomIntra (p_Vid, currMB->mbAddrX);    // Forced Pseudo-Random Intra

  //===== Setup Macroblock encoding parameters =====
  init_enc_mb_params(currMB, &enc_mb, intra);
  //将MD的lambda设置为ME的（下列注释内）：
  //enc_mb.lambda_md = enc_mb.lambda_me[0]; //直接乘1.1
  //enc_mb.lambda_mdfp = enc_mb.lambda_mf[0]; //int转double转int
  //enc_mb.lambda_md = 1.1 * enc_mb.lambda_me[0]; // ^{  ^n   ^x1.1
  //enc_mb.lambda_mdfp = (int)((double)1.1 * (double)(enc_mb.lambda_mf[0])); //int   double   int
  if (p_Inp->AdaptiveRounding)
  {
    reset_adaptive_rounding(p_Vid);
  }

  if (currSlice->mb_aff_frame_flag)
  {
    reset_mb_nz_coeff(p_Vid, currMB->mbAddrX);
  }

  //=====   S T O R E   C O D I N G   S T A T E   ===== 保存编码状态
  //---------------------------------------------------
  currSlice->store_coding_state (currMB, currSlice->p_RDO->cs_cm);

  //帧间预测,计算其选择合适模式后的开销  (根据拉格朗日率失真准则)  
  if (!intra)
  {
    //B帧的直接模式预测 
    //===== set skip/direct motion vectors =====
    if (enc_mb.valid[0])
    {
      if (bslice)//检查是否是B帧：是否没有direct模式 但有skip模式
        currSlice->Get_Direct_Motion_Vectors (currMB);
      else 
        FindSkipModeMotionVector (currMB); //SKIP MODE
    }
    if (p_Inp->CtxAdptLagrangeMult == 1)
    {
      get_initial_mb16x16_cost(currMB);
    }

    //宏块级的运动估计
    //===== MOTION ESTIMATION FOR 16x16, 16x8, 8x16 BLOCKS =====
    for (mode = 1; mode < 4; mode++)
    {
      best.mode = (char) mode;
      best.bipred = 0;
      b8x8info->best[mode][0].bipred = 0;
      
      if (enc_mb.valid[mode])
      {
        for (cost=0, block=0; block<(mode==1?1:2); block++) //16*16 分割方式只需要计算一次，16*8 和 8*16 分割方式需要计算两次  
        {
          update_lambda_costs(currMB, &enc_mb, lambda_mf); //更新ME的lambda：lambda_mf
          //做ME的函数调用过程：(mv_search.c)PartitionMotionSearch() -> (mv_search.c)BlockMotionSearch() 
          //               -> {currMB->IntPelME()} -> (me_fullsearch.c)full_search_motion_estimation()
          PartitionMotionSearch (currMB, mode, block, lambda_mf); // 对 16*16、16*8、8*16 分割方式各做了一次ME

          /* === DEBUG 打印出所有Lambda ===
          printf("\n===================\n lambda_mf(outside):%d, %d, %d", lambda_mf[0], lambda_mf[1], lambda_mf[2]);
          printf("\nIn enc_mb:\nlambda_md/mdfp: %f, %d", enc_mb.lambda_md, enc_mb.lambda_mdfp);
          printf("\nlambda_me/mf: %f, %f, %f, %d, %d, %d \n", enc_mb.lambda_me[0], enc_mb.lambda_me[1], enc_mb.lambda_me[2], enc_mb.lambda_mf[0], enc_mb.lambda_mf[1], enc_mb.lambda_mf[2]);
          */

          //--- set 4x4 block indices (for getting MV) ---
          j = (block==1 && mode==2 ? 2 : 0);  //如果现在处理的是 16*8 分割方式的第 2 个分割，则 j=2  
          i = (block==1 && mode==3 ? 2 : 0);  //如果现在处理的是 8*16 分割方式的第 2 个分割，则 i=2  

          //--- get cost and reference frame for List 0 prediction ---
          bmcost[LIST_0] = DISTBLK_MAX;     //bmcost用来记录(前向,后向和双向)最佳参考帧对应的代价
          list_prediction_cost(currMB, LIST_0, block, mode, &enc_mb, bmcost, best.ref);

          if (bslice)
          {
            //--- get cost and reference frame for List 1 prediction ---
            bmcost[LIST_1] = DISTBLK_MAX;
            list_prediction_cost(currMB, LIST_1, block, mode, &enc_mb, bmcost, best.ref);

            // Compute bipredictive cost between best list 0 and best list 1 references
            list_prediction_cost(currMB, BI_PRED, block, mode, &enc_mb, bmcost, best.ref);

            // currently Bi predictive ME is only supported for modes 1, 2, 3 and ref 0
            if (is_bipred_enabled(p_Vid, mode))
            {
              get_bipred_cost(currMB, mode, block, i, j, &best, &enc_mb, bmcost);
            }
            else
            {
              bmcost[BI_PRED_L0] = DISTBLK_MAX;
              bmcost[BI_PRED_L1] = DISTBLK_MAX;
            }

            // Determine prediction list based on mode cost
            determine_prediction_list(bmcost, &best, &cost);
          }
          else // if (bslice)
          {
            best.pdir = 0;
            cost      += bmcost[LIST_0];
          }

          assign_enc_picture_params(currMB, mode, &best, 2 * block);

          //----- set reference frame and direction parameters -----
          set_block8x8_info(b8x8info, mode, block, &best);

          //--- set reference frames and motion vectors ---
          if (mode>1 && block == 0)
            currSlice->set_ref_and_motion_vectors (currMB, motion, &best, block);
        } // for (block=0; block<(mode==1?1:2); block++)
        if (cost < min_cost)
        {
          md_best.mode = (byte) mode;
          md_best.cost = cost;
          currMB->best_mode = (short) mode;
          min_cost  = cost;
          if (p_Inp->CtxAdptLagrangeMult == 1)
          {
            adjust_mb16x16_cost(currMB, cost);
          }
        }
      } // if (enc_mb.valid[mode])
    } // for (mode=1; mode<4; mode++)


    //现在宏块级运动估计结束, md_best.mode保存了最佳的宏块模式, min_cost记录其对应的代价  
    //开始亚宏块级运动估计  
    if (enc_mb.valid[P8x8])
    {    
      currMB->valid_8x8 = FALSE;

      if (p_Inp->Transform8x8Mode)
      {
        ResetRD8x8Data(p_Vid, p_RDO->tr8x8);
        currMB->luma_transform_size_8x8_flag = TRUE; //switch to 8x8 transform size
        //===========================================================
        // Check 8x8 partition with transform size 8x8
        //===========================================================
        //=====  LOOP OVER 8x8 SUB-PARTITIONS  (Motion Estimation & Mode Decision) =====
        // 遍历16x16宏块的4个8x8块  
        for (block = 0; block < 4; block++)
        {
          currSlice->submacroblock_mode_decision(currMB, &enc_mb, p_RDO->tr8x8, p_RDO->cofAC8x8ts[block], block, &cost); //亚宏块的运动估计
          if(!currMB->valid_8x8)
            break;
          set_subblock8x8_info(b8x8info, P8x8, block, p_RDO->tr8x8);
        } 

      }// if (p_Inp->Transform8x8Mode)

      currMB->valid_4x4 = FALSE;
      if (p_Inp->Transform8x8Mode != 2)
      {
        currMB->luma_transform_size_8x8_flag = FALSE; //switch to 8x8 transform size
        ResetRD8x8Data(p_Vid, p_RDO->tr4x4);
        //=================================================================
        // Check 8x8, 8x4, 4x8 and 4x4 partitions with transform size 4x4
        //=================================================================
        //=====  LOOP OVER 8x8 SUB-PARTITIONS  (Motion Estimation & Mode Decision) =====
        for (block = 0; block < 4; block++)
        {
          currSlice->submacroblock_mode_decision(currMB, &enc_mb, p_RDO->tr4x4, p_RDO->coefAC8x8[block], block, &cost); //亚宏块的运动估计
          if(!currMB->valid_4x4)
            break;
          set_subblock8x8_info(b8x8info, P8x8, block, p_RDO->tr4x4);
        }
      }// if (p_Inp->Transform8x8Mode != 2)

      if (p_Inp->RCEnable)
        rc_store_diff(currSlice->diffy, &p_Vid->pCurImg[currMB->opix_y], currMB->pix_x, mb_pred);

      p_Vid->giRDOpt_B8OnlyFlag = FALSE;
    }
  }
  else // if (!intra)
  {
    min_cost = DISTBLK_MAX;
  }

  // Set Chroma mode
  set_chroma_pred_mode(currMB, enc_mb, mb_available, chroma_pred_mode_range);

  //========= C H O O S E   B E S T   M A C R O B L O C K   M O D E =========
  //-------------------------------------------------------------------------

  // 帧内预测的计算
  // c_ipred_mode 储存帧内预测的模式
  for (currMB->c_ipred_mode = chroma_pred_mode_range[0]; currMB->c_ipred_mode<=chroma_pred_mode_range[1]; currMB->c_ipred_mode++)
  {
    // bypass if c_ipred_mode is not allowed -- 若当前 c_ipred_mode 模式不被允许，则考虑下一个 c_ipred_mode 模式;  
    if ( (p_Vid->yuv_format != YUV400) &&
      (  ((!intra || !p_Inp->IntraDisableInterOnly) && p_Inp->ChromaIntraDisable == 1 && currMB->c_ipred_mode!=DC_PRED_8) 
      || (currMB->c_ipred_mode == VERT_PRED_8 && !mb_available[0]) 
      || (currMB->c_ipred_mode == HOR_PRED_8  && !mb_available[1]) 
      || (currMB->c_ipred_mode == PLANE_8     && (!mb_available[1] || !mb_available[0] || !mb_available[2]))))
      continue;        

    //===== GET BEST MACROBLOCK MODE =====
    for (index=0; index < max_index; index++)
    {
      mode = mb_mode_table[index]; //mb_mode_table[7]  = {0, 1, 2, 3, P8x8, I16MB, I4MB};
      //printf("mode %d %7.3f", mode, (double) currMB->min_rdcost);
      if (enc_mb.valid[mode])
      {
        //printf(" mode %d is valid", mode);
        if (p_Vid->yuv_format != YUV400)
        {           
          currMB->i16mode = 0; 
        }

        // Skip intra modes in inter slices if best mode is inter <P8x8 with cbp equal to 0    
        if (currSlice->P444_joined)
        {
          if (p_Inp->SkipIntraInInterSlices && !intra && mode >= I16MB 
            && currMB->best_mode <=3 && currMB->best_cbp == 0 && currSlice->cmp_cbp[1] == 0 && currSlice->cmp_cbp[2] == 0 && (currMB->min_rdcost < weighted_cost(enc_mb.lambda_mdfp,5)))
            continue;
        }
        else
        {
          if (p_Inp->SkipIntraInInterSlices)
          {
            if (!intra && mode >= I4MB)
            {
              if (currMB->best_mode <=3 && currMB->best_cbp == 0 && (currMB->min_rdcost < weighted_cost(enc_mb.lambda_mdfp, 5)))
              {
                continue;
              }
              else if (currMB->best_mode == 0 && (currMB->min_rdcost < weighted_cost(enc_mb.lambda_mdfp,6)))
              {
                continue;
              }
            }
          }
        }

        compute_mode_RD_cost(currMB, &enc_mb, (short) mode, &inter_skip);
        
      }
      //printf(" best %d %7.2f\n", currMB->best_mode, (double) currMB->min_rdcost);
    }// for (index=0; index<max_index; index++)
  }// for (currMB->c_ipred_mode=DC_PRED_8; currMB->c_ipred_mode<=chroma_pred_mode_range[1]; currMB->c_ipred_mode++)                     

  restore_nz_coeff(currMB);

  intra1 = is_intra(currMB);

  //=====  S E T   F I N A L   M A C R O B L O C K   P A R A M E T E R S ======
  //---------------------------------------------------------------------------
  update_qp_cbp_tmp(currMB, p_RDO->cbp);
  currSlice->set_stored_mb_parameters (currMB);

  // Rate control
  if(p_Inp->RCEnable && p_Inp->RCUpdateMode <= MAX_RC_MODE)
    rc_store_mad(currMB);


  //===== Decide if this MB will restrict the reference frames =====
  if (p_Inp->RestrictRef)
    update_refresh_map(currMB, intra, intra1);
}


