#include "../../../../../MarlinCore.h"
#if ENABLED(TFT_LITTLE_VGL_UI)
#include "lv_conf.h"
#include "../inc/draw_ui.h"
#include "../../../../../module/temperature.h"
#include "../../../../../module/motion.h"
#include "../../../../../sd/cardreader.h"
#include "../../../../../gcode/queue.h"
#if ENABLED(POWER_LOSS_RECOVERY)
#include "../../../../../feature/powerloss.h"
#endif

#include "../../../../../gcode/gcode.h"
#include "../../../../../module/planner.h"


extern uint32_t To_pre_view;
extern uint8_t flash_preview_begin;
extern uint8_t default_preview_flg;
extern uint8_t gcode_preview_over;

void printer_state_polling()
{
	if(uiCfg.print_state == PAUSING)
	{
		#if ENABLED(SDSUPPORT)
		if( !planner.has_blocks_queued() &&  card.getIndex()>MIN_FILE_PRINTED)  
			uiCfg.waitEndMoves++;
		
		if(uiCfg.waitEndMoves > 20)
		{
			uiCfg.waitEndMoves = 0;
			planner.synchronize();
			gcode.process_subcommands_now_P(PSTR("M25"));
			if(gCfgItems.pausePosZ != (float)-1)
			{
				gcode.process_subcommands_now_P(PSTR("G91"));
				memset(public_buf_l,0,sizeof(public_buf_l));
				sprintf(public_buf_l,"G1 Z%.1f",gCfgItems.pausePosZ);
				gcode.process_subcommands_now_P(PSTR(public_buf_l));
				gcode.process_subcommands_now_P(PSTR("G90"));
			}
			if(gCfgItems.pausePosX != (float)-1 && gCfgItems.pausePosY != (float)-1)
			{
				memset(public_buf_l,0,sizeof(public_buf_l));
				sprintf(public_buf_l,"G1 X%.1f Y%.1f",gCfgItems.pausePosX,gCfgItems.pausePosY);
				gcode.process_subcommands_now_P(PSTR(public_buf_l));
			}
			uiCfg.print_state = PAUSED;
			
			//#if ENABLED(POWER_LOSS_RECOVERY)
		      //if (recovery.enabled) recovery.save(true);
		    //#endif
			gCfgItems.pause_reprint = 1;
			update_spi_flash();
		}
	  	#endif
	}
	else
	{
		uiCfg.waitEndMoves = 0;
	}
	
	if(uiCfg.print_state == PAUSED)
	{
		
	}
	if(uiCfg.print_state == RESUMING)
	{
		if (IS_SD_PAUSED())
		{
			gcode.process_subcommands_now_P(PSTR("M24"));
			gcode.process_subcommands_now_P(PSTR("G91"));
			gcode.process_subcommands_now_P(PSTR("G1 Z-5"));
			gcode.process_subcommands_now_P(PSTR("G90"));
			uiCfg.print_state = WORKING;
			start_print_time();
			
			gCfgItems.pause_reprint = 0;
			update_spi_flash();
		}
	}
	if(uiCfg.print_state == REPRINTED)
	{
		memset(public_buf_m,0,sizeof(public_buf_m));
		  #if HOTENDS
		    HOTEND_LOOP() {
		      const int16_t et = recovery.info.target_temperature[e];
		      if (et) {
		        #if HOTENDS > 1
		          sprintf_P(public_buf_m, PSTR("T%i"), e);
		          gcode.process_subcommands_now(public_buf_m);
		        #endif
		        sprintf_P(public_buf_m, PSTR("M109 S%i"), et);
		        gcode.process_subcommands_now(public_buf_m);
		      }
		    }
		  #endif

		if(gCfgItems.pause_reprint == 1)
		{
			gcode.process_subcommands_now_P(PSTR("G91"));
			gcode.process_subcommands_now_P(PSTR("G1 Z-5"));
			gcode.process_subcommands_now_P(PSTR("G90"));
		}
		recovery.resume();
		
		uiCfg.print_state = WORKING;
		start_print_time();

		gCfgItems.pause_reprint = 0;
		update_spi_flash();
	}
	if(uiCfg.print_state == WORKING)
	{
		filament_check();
	}
}


void filament_pin_setup()
{
	#if PIN_EXISTS(MT_DET_1)
	pinMode(MT_DET_1_PIN, INPUT_PULLUP);
	#endif
	
	#if PIN_EXISTS(MT_DET_2)
	pinMode(MT_DET_2_PIN, INPUT_PULLUP);
	#endif
	
	#if PIN_EXISTS(MT_DET_3)
	pinMode(MT_DET_3_PIN, INPUT_PULLUP);
	#endif
}

void filament_check()
{
	const int FIL_DELAY = 20;
	#if PIN_EXISTS(MT_DET_1)
	static int fil_det_count_1 = 0;   
    if (!READ(MT_DET_1_PIN) && !MT_DET_PIN_INVERTING)
      fil_det_count_1++;
    else if (READ(MT_DET_1_PIN) && MT_DET_PIN_INVERTING)
      fil_det_count_1++;
    else if (fil_det_count_1 > 0)
      fil_det_count_1--;

	if (!READ(MT_DET_1_PIN) && !MT_DET_PIN_INVERTING)
      fil_det_count_1++;
    else if (READ(MT_DET_1_PIN) && MT_DET_PIN_INVERTING)
      fil_det_count_1++;
    else if (fil_det_count_1 > 0)
      fil_det_count_1--;

	#endif

	#if PIN_EXISTS(MT_DET_2)
	static int fil_det_count_2 = 0;  
    if (!READ(MT_DET_2_PIN) && !MT_DET_PIN_INVERTING)
      fil_det_count_2++;
    else if (READ(MT_DET_2_PIN) && MT_DET_PIN_INVERTING)
      fil_det_count_2++;
    else if (fil_det_count_2 > 0)
      fil_det_count_2--;

	if (!READ(MT_DET_2_PIN) && !MT_DET_PIN_INVERTING)
      fil_det_count_2++;
    else if (READ(MT_DET_2_PIN) && MT_DET_PIN_INVERTING)
      fil_det_count_2++;
    else if (fil_det_count_2 > 0)
      fil_det_count_2--;

	#endif

	#if PIN_EXISTS(MT_DET_3)
	static int fil_det_count_3 = 0;  
    if (!READ(MT_DET_3_PIN) && !MT_DET_PIN_INVERTING)
      fil_det_count_3++;
    else if (READ(MT_DET_3_PIN) && MT_DET_PIN_INVERTING)
      fil_det_count_3++;
    else if (fil_det_count_3 > 0)
      fil_det_count_3--;

	if (!READ(MT_DET_3_PIN) && !MT_DET_PIN_INVERTING)
      fil_det_count_3++;
    else if (READ(MT_DET_3_PIN) && MT_DET_PIN_INVERTING)
      fil_det_count_3++;
    else if (fil_det_count_3 > 0)
      fil_det_count_3--;

	#endif
	
    if (
	#if PIN_EXISTS(MT_DET_1)
	fil_det_count_1 >= FIL_DELAY
	#else
	false
	#endif
	#if PIN_EXISTS(MT_DET_2)
	 || fil_det_count_2 >= FIL_DELAY
	#endif
	#if PIN_EXISTS(MT_DET_3)
	 || fil_det_count_3 >= FIL_DELAY
	#endif
	)
	{
	 clear_cur_ui();
	 card.pauseSDPrint();
	 stop_print_time();
	 uiCfg.print_state = PAUSING;
	 
	 if(gCfgItems.from_flash_pic == 1)
		flash_preview_begin = 1;
	else
		default_preview_flg = 1; 
	
	 lv_draw_printing();
    }
}


#endif
