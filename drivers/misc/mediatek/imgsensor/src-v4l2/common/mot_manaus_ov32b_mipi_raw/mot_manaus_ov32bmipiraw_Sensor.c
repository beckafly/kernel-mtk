// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 mot_manaus_ov32bmipiraw_Sensor.c
 *
 * Project:
 * --------
 *	 MANAUS
 *
 * Description:
 * ------------
 *	 Source code of Sensor driver
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>
#include <linux/module.h>
#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define_v4l2.h"
#include "kd_imgsensor_errcode.h"

#include "mot_manaus_ov32bmipiraw_Sensor.h"

#include "adaptor-subdrv.h"
#include "adaptor-i2c.h"


#define read_cmos_sensor_8(...) subdrv_i2c_rd_u8(__VA_ARGS__)
#define write_cmos_sensor_8(...) subdrv_i2c_wr_u8(__VA_ARGS__)
#define read_cmos_sensor(...) subdrv_i2c_rd_u16(__VA_ARGS__)
#define write_cmos_sensor(...) subdrv_i2c_wr_u16(__VA_ARGS__)
#define mot_manaus_ov32b_write_cmos_sensor(...) subdrv_i2c_wr_regs_u8(__VA_ARGS__)

/**********************Modify Following Strings for Debug**********************/
#define PFX "mot_manaus_ov32b"
static int mot_ov32b_camera_debug = 1;
module_param(mot_ov32b_camera_debug,int, 0644);

static int mot_ov32b_fusion_talk_en = 0;
module_param(mot_ov32b_fusion_talk_en,int, 0644);
#define LOG_INF(format, args...)        do { if (mot_ov32b_camera_debug ) { pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args); } } while(0)
#define LOG_INF_N(format, args...)     pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args)
#define LOG_ERR(format, args...)       pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args)

/***********************   Modify end    **************************************/
#define per_frame 1
#define MULTI_WRITE 1
#define ENABLE_PDAF 0
#define USE_REMOSAIC 0

static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = MOT_MANAUS_OV32B_SENSOR_ID,
	.checksum_value = 0x4f1b1d5e,
	.pre = {
		.pclk = 115200000,
		.linelength = 1440,
		.framelength = 2664,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,
		.grabwindow_height = 2448,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 340800000,
	},
	.cap = {
		.pclk = 115200000,
		.linelength = 1440,
		.framelength = 2664,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,
		.grabwindow_height = 2448,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 340800000,
	},
	.normal_video = {
		.pclk = 115200000,
		.linelength = 1440,
		.framelength = 2664,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,
		.grabwindow_height = 1836,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 340800000,
	},
	.hs_video = {
		.pclk = 115200000,
		.linelength = 1440,
		.framelength = 2664,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,
		.grabwindow_height = 2448,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 340800000,
	},
	.slim_video = {
		.pclk = 115200000,
		.linelength = 1440,
		.framelength = 2664,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,
		.grabwindow_height = 2448,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 340800000,
	},
	.custom1 = {
		.pclk = 115200000,
		.linelength = 720,
		.framelength = 2664,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,
		.grabwindow_height = 1836,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 600,
		.mipi_pixel_rate = 681600000,
	},
	.margin = 31,
	.min_shutter = 16,
	.min_gain = BASEGAIN,
	.max_gain = BASEGAIN * 15.9375,//1024 * 15.9375=16320
	.min_gain_iso = 100,
	.gain_step = 1,
	.gain_type = 1,	//OV
	.max_frame_length = 0xffffff,


#if per_frame
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,
#else
    	.ae_shut_delay_frame = 0,
    	.ae_sensor_gain_delay_frame = 1,
    	.ae_ispGain_delay_frame = 2,
#endif
	.ihdr_support = 0,	/* 1, support; 0,not support */
	.ihdr_le_firstline = 0,	/* 1,le first ; 0, se first */
	.temperature_support = 0, /* 1, support; 0,not support */
	.sensor_mode_num = 6,	/* support sensor mode num */
	.frame_time_delay_frame = 2,
	.cap_delay_frame = 2,	/* enter capture delay frame num */
	.pre_delay_frame = 2,	/* enter preview delay frame num */
	.video_delay_frame = 2,	/* enter video delay frame num */
	.hs_video_delay_frame = 2, /* enter high speed video  delay frame num */
	.slim_video_delay_frame = 2,	/* enter slim video delay frame num */
	.custom1_delay_frame = 2,		//enter custom1 delay frame num
	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = {0x20, 0xff},
	.i2c_speed = 1000,
};


/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[6] = {
    { 6528, 4896,   0,   0, 6528, 4896,  3264, 2448,  0,  0, 3264, 2448, 0, 0, 3264, 2448},       // preview
    { 6528, 4896,   0,   0, 6528, 4896,  3264, 2448,  0,  0, 3264, 2448, 0, 0, 3264, 2448},       // capture
    { 6528, 4896,   0,   612, 6528, 3672,  3264, 1836,  0,  0, 3264, 1836, 0, 0, 3264, 1836},       // VIDEO
    { 6528, 4896,   0,   0, 6528, 4896,  3264, 2448,  0,  0, 3264, 2448, 0, 0, 3264, 2448},       // hight speed video
    { 6528, 4896,   0,   0, 6528, 4896,  3264, 2448,  0,  0, 3264, 2448, 0, 0, 3264, 2448},       // slim video
    { 6528, 4896,   0,   612, 6528, 3672,  3264, 1836,  0,  0, 3264, 1836, 0, 0, 3264, 1836},       // Custom1
};

static kal_uint16 addr_data_pair_init_mot_manaus_ov32b[] = {
#include "setting/mot_manaus_ov32b_init.h"
};

static kal_uint16 addr_data_pair_preview_mot_manaus_ov32b[] = {
#include "setting/OV32B40_3264X2448_MIPI852_30FPS_NORMAL_noPD.h"
};

static kal_uint16 addr_data_pair_video_mot_manaus_ov32b[] = {
#include "setting/OV32B40_3264X1836_MIPI852_30FPS_NORMAL_noPD.h"
};

static kal_uint16 addr_data_pair_custom1_mot_manaus_ov32b[] = {
#include"setting/OV32B40_3264X1836_MIPI1704_60FPS_NORMAL_noPD.h"
};

static kal_uint32 ov32b_ana_gain_table[] = {
#include "setting/mot_ov32b_gain_table.h"
};

// static void set_normal_mode(struct subdrv_ctx *ctx)
// {
//      int ret=0;
//      ctx->i2c_client->addr=0x30;
//      ctx->i2c_write_id=0x30;
//      ret=write_cmos_sensor_8(ctx, 0x1000, 0x00);
//      ret=write_cmos_sensor_8(ctx, 0x1001, 0x04);
//      msleep(1);
//      ctx->i2c_client->addr=0x20;
//      ctx->i2c_write_id=0x20;
//      ret=write_cmos_sensor_8(ctx, 0x0103, 0x01);
//      msleep(1);
// }
static void set_dummy(struct subdrv_ctx *ctx)
{
	LOG_INF("dummyline = %d, dummypixels = %d\n",
		ctx->dummy_line, ctx->dummy_pixel);
	write_cmos_sensor_8(ctx, 0x380c, ctx->line_length >> 8);
	write_cmos_sensor_8(ctx, 0x380d, ctx->line_length & 0xFF);
	write_cmos_sensor_8(ctx, 0x380e, ctx->frame_length >> 8);
	write_cmos_sensor_8(ctx, 0x380f, ctx->frame_length & 0xFF);
}

static inline void extend_frame_length(struct subdrv_ctx *ctx, kal_uint32 frame_length)
{
	/* Extend frame length*/
	write_cmos_sensor_8(ctx, 0x3840, (frame_length >> 16) & 0xFF); //0x7f
	write_cmos_sensor_8(ctx, 0x380e, (frame_length >> 8) & 0xFF); //0x7f
	write_cmos_sensor_8(ctx, 0x380f, frame_length & 0xFF);
}

static inline void update_shutter(struct subdrv_ctx *ctx, kal_uint32 shutter)
{
	kal_uint32 long_shutter;
	/*1s=1000000000  tline=12500  1000000000/12500=80000*/
	if (shutter > 70000) {
		/* long exposure
		  *ov32b  default binning=2,
		  *In the tuning file camera_AE_custom_transfotm.cpp:
		  *u4Eposuretime = u4Eposuretime / u4BinningSumRatio
		  */
		long_shutter = shutter;
		write_cmos_sensor_8(ctx, 0x3500, (long_shutter >> 16) & 0xFF);
		write_cmos_sensor_8(ctx, 0x3501, (long_shutter >> 8) & 0xFF);
		write_cmos_sensor_8(ctx, 0x3502, (long_shutter)&0xFF);
	} else {
		/*update shutter*/
		write_cmos_sensor_8(ctx, 0x3500, (shutter >> 16) & 0xFF);
		write_cmos_sensor_8(ctx, 0x3501, (shutter >> 8) & 0xFF);
		write_cmos_sensor_8(ctx, 0x3502, (shutter)&0xFF);
	}
}

static void set_max_framerate(struct subdrv_ctx *ctx, UINT16 framerate, kal_bool min_framelength_en)
{
	kal_uint32 frame_length = ctx->frame_length;

	LOG_INF("framerate = %d, min framelength should enable %d\n", framerate,
		min_framelength_en);

	frame_length = ctx->pclk / framerate * 10 / ctx->line_length;
	if (frame_length >= ctx->min_frame_length)
		ctx->frame_length = frame_length;
	else
		ctx->frame_length = ctx->min_frame_length;

	ctx->dummy_line = ctx->frame_length - ctx->min_frame_length;

	if (ctx->frame_length > imgsensor_info.max_frame_length) {
		ctx->frame_length = imgsensor_info.max_frame_length;
		ctx->dummy_line = ctx->frame_length - ctx->min_frame_length;
	}
	if (min_framelength_en)
		ctx->min_frame_length = ctx->frame_length;
} /*	set_max_framerate  */

//check ok
static void write_shutter(struct subdrv_ctx *ctx, kal_uint32 shutter)
{
	kal_uint16 realtime_fps = 0;

	if (shutter > ctx->min_frame_length - imgsensor_info.margin)
		ctx->frame_length = shutter + imgsensor_info.margin;
	else
		ctx->frame_length = ctx->min_frame_length;

	if (ctx->frame_length > imgsensor_info.max_frame_length)
		ctx->frame_length = imgsensor_info.max_frame_length;

	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ?
				(imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

	if (ctx->autoflicker_en) {
		realtime_fps = ctx->pclk / ctx->line_length * 10 / ctx->frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(ctx, 296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(ctx, 146, 0);
		else
			extend_frame_length(ctx, ctx->frame_length);
	} else
		extend_frame_length(ctx, ctx->frame_length);
	/* Update Shutter*/
	update_shutter(ctx, shutter);

	LOG_INF("ov32b  shutter = %d, framelength = %d\n", shutter,
		ctx->frame_length);
} /*	write_shutter  */

/*
 ************************************************************************
 * FUNCTION
 *	set_shutter
 *
 * DESCRIPTION
 *	This function set e-shutter of sensor to change exposure time.
 *
 * PARAMETERS
 *	iShutter : exposured lines
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 ************************************************************************
 */
static void set_shutter(struct subdrv_ctx *ctx, kal_uint32 shutter)
{
	LOG_INF("set_shutter");
	ctx->shutter = shutter;

	write_shutter(ctx, shutter);
} /*	set_shutter */

//check ok
static void set_frame_length(struct subdrv_ctx *ctx, kal_uint32 frame_length)
{
	if (frame_length > 1)
		ctx->frame_length = frame_length;

	if (ctx->frame_length > imgsensor_info.max_frame_length)
		ctx->frame_length = imgsensor_info.max_frame_length;
	if (ctx->min_frame_length > ctx->frame_length)
		ctx->frame_length = ctx->min_frame_length;

	/* Extend frame length */
	extend_frame_length(ctx, ctx->frame_length);
	LOG_INF("Framelength: set=%d/input=%d/min=%d\n", ctx->frame_length,
		frame_length, ctx->min_frame_length);
}
//check ok
static void set_shutter_frame_length(struct subdrv_ctx *ctx, kal_uint32 shutter, kal_uint32 target_frame_length)
{
	if (target_frame_length > 1)
		ctx->dummy_line = target_frame_length - ctx->frame_length;
	ctx->frame_length = ctx->frame_length + ctx->dummy_line;
	ctx->min_frame_length = ctx->frame_length;
	set_shutter(ctx, shutter);
}

static kal_uint16 gain2reg(struct subdrv_ctx *ctx, kal_uint32 gain)
{
	kal_uint16 reg_gain = 0x0;
	gain = gain * 256 / BASEGAIN;
	reg_gain = (kal_uint16)gain;
	return reg_gain;
}

/*
 ************************************************************************
 * FUNCTION
 *	set_gain
 *
 * DESCRIPTION
 *	This function is to set global gain to sensor.
 *
 * PARAMETERS
 *	iGain : sensor global gain(base: 0x400)
 *
 * RETURNS
 *	the actually gain set to sensor.
 *
 * GLOBALS AFFECTED
 *
 ************************************************************************
 */
static kal_uint32 set_gain(struct subdrv_ctx *ctx, kal_uint32 gain)
{
	kal_uint16 reg_gain;

	if (gain < imgsensor_info.min_gain || gain > imgsensor_info.max_gain) {
		LOG_INF_N("Error gain setting");

		if (gain < imgsensor_info.min_gain)
			gain = imgsensor_info.min_gain;
		else if (gain > imgsensor_info.max_gain)
			gain = imgsensor_info.max_gain;
	}
	reg_gain = gain2reg(ctx, gain);
	ctx->gain = reg_gain;
	LOG_INF("gain = %d , reg_gain = 0x%x  \n", gain, reg_gain);
	write_cmos_sensor_8(ctx, 0x3508, (reg_gain >> 8) & 0xFF);
	write_cmos_sensor_8(ctx, 0x3509, reg_gain & 0xFF);
	return gain;
} /*	set_gain  */

static kal_uint32 streaming_control(struct subdrv_ctx *ctx, kal_bool enable)
{
	unsigned int tmp;
	LOG_INF("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);

	if (enable)
		write_cmos_sensor_8(ctx, 0x0100, 0x01); //stream on
	else {
		tmp = read_cmos_sensor_8(ctx, 0x0100);
		if (tmp & 0x01)
			write_cmos_sensor_8(ctx, 0x0100, 0x00); // stream off
	}
	return ERROR_NONE;
}
#define OV32B_EEPROM_IIC_ADDR 0xA2
#define OV32B_SENSOR_IIC_ADDR 0x20
#define OV32B_FUSION_TALK_EEPROM_OFFSET 0x07E1
#define OV32B_FUSION_TALK_REG_OFFSET 0x6AD0
#define OV32B_XTLK_BYTES 288
static u8 ov32b_xtlk_buf[OV32B_XTLK_BYTES];
static u8 fusion_talk_ready = 0;
static void ov32b_fusion_talk_read(struct subdrv_ctx *ctx)
{
	int ret = 0;

	memset(ov32b_xtlk_buf, 0x0, sizeof(ov32b_xtlk_buf));
	LOG_INF("%s: xtalk read start", __func__);
	ret = adaptor_i2c_rd_p8(ctx->i2c_client, OV32B_EEPROM_IIC_ADDR >> 1,
				OV32B_FUSION_TALK_EEPROM_OFFSET, ov32b_xtlk_buf, sizeof(ov32b_xtlk_buf));
	LOG_INF("xtalk read end");
	if (ret < 0) {
		LOG_ERR("sequential read fusion talk failed, ret: %d\n", ret);
	} else {
		fusion_talk_ready = 1;
		LOG_INF("sequential read fusion talk success\n");
	}
}

static void ov32b_fusion_talk_apply(struct subdrv_ctx *ctx)
{
	/*Registers need write:
	    0x6AD0 ~ 0x6BEF     288 bytes
	*/
	int ret = 0;
	if (!fusion_talk_ready) { //Try again if power on reading failed
		LOG_INF("fusion talk read retry.");
		ov32b_fusion_talk_read(ctx);
	}
	if (fusion_talk_ready) {
		ret = adaptor_i2c_wr_seq_p8(ctx->i2c_client, OV32B_SENSOR_IIC_ADDR >> 1,
					    OV32B_FUSION_TALK_REG_OFFSET, ov32b_xtlk_buf, OV32B_XTLK_BYTES);
		if (ret < 0) {
			LOG_ERR(" Sequential Apply xtalk failed, ret: %d\n", ret);
		} else {
			LOG_INF("%s: Sequential Apply fusion  talk success\n", __func__);
		}
	} else {
		LOG_ERR("read fusion talk failed, ret: %d\n", ret);
	}
	return;
}

/*************************************************************************
 * FUNCTION
 *    night_mode
 *
 * DESCRIPTION
 *    This function night mode of sensor.
 *
 * PARAMETERS
 *    bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
 *
 * RETURNS
 *    None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static void night_mode(struct subdrv_ctx *ctx, kal_bool enable)
{
/*No Need to implement this function*/
}    /*    night_mode    */


/*	sensor_init  */
static void sensor_init(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");
	if (1 == mot_ov32b_fusion_talk_en) {
		mot_manaus_ov32b_write_cmos_sensor(ctx, addr_data_pair_init_mot_manaus_ov32b,
			sizeof(addr_data_pair_init_mot_manaus_ov32b) / sizeof(kal_uint16));
		LOG_INF("Applying fusion talk...");
		ov32b_fusion_talk_apply(ctx);
	} else {
		mot_manaus_ov32b_write_cmos_sensor(ctx, addr_data_pair_init_mot_manaus_ov32b,
			sizeof(addr_data_pair_init_mot_manaus_ov32b) / sizeof(kal_uint16));
	}
	LOG_INF("X\n");
}

/*	preview_setting  */
static void preview_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");
	mot_manaus_ov32b_write_cmos_sensor(ctx, addr_data_pair_preview_mot_manaus_ov32b,
		sizeof(addr_data_pair_preview_mot_manaus_ov32b) / sizeof(kal_uint16));
	LOG_INF("X\n");
}

/*	capture_setting  */
static void capture_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");
	mot_manaus_ov32b_write_cmos_sensor(ctx, addr_data_pair_preview_mot_manaus_ov32b,
		sizeof(addr_data_pair_preview_mot_manaus_ov32b) / sizeof(kal_uint16));
	LOG_INF("X\n");
}

/*	normal_video_setting  */
static void video_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");
	mot_manaus_ov32b_write_cmos_sensor(ctx, addr_data_pair_video_mot_manaus_ov32b,
		sizeof(addr_data_pair_video_mot_manaus_ov32b) / sizeof(kal_uint16));
	LOG_INF("X\n");
}

/*	hs_video_setting  */
static void hs_video_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");
	mot_manaus_ov32b_write_cmos_sensor(ctx, addr_data_pair_preview_mot_manaus_ov32b,
		sizeof(addr_data_pair_preview_mot_manaus_ov32b) / sizeof(kal_uint16));
	LOG_INF("X\n");
}

/*	slim_video_setting  */
static void slim_video_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");
	mot_manaus_ov32b_write_cmos_sensor(ctx, addr_data_pair_preview_mot_manaus_ov32b,
		sizeof(addr_data_pair_preview_mot_manaus_ov32b) / sizeof(kal_uint16));
	LOG_INF("X\n");
}

/*	custom1_setting  */
static void custom1_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");
	mot_manaus_ov32b_write_cmos_sensor(ctx, addr_data_pair_custom1_mot_manaus_ov32b,
		sizeof(addr_data_pair_custom1_mot_manaus_ov32b) / sizeof(kal_uint16));
	LOG_INF("X\n");
}

static kal_uint32 return_sensor_id(struct subdrv_ctx *ctx)
{
	return ((read_cmos_sensor_8(ctx, 0x300B) << 8) | read_cmos_sensor_8(ctx, 0x300c));
}

/*************************************************************************
 * FUNCTION
 *	get_imgsensor_id
 *
 * DESCRIPTION
 *	This function get the sensor ID
 *
 * PARAMETERS
 *	*sensorID : return the sensor ID
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 ************************************************************************
 */
static int get_imgsensor_id(struct subdrv_ctx *ctx, UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	// set_normal_mode(ctx);
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		ctx->i2c_write_id = imgsensor_info.i2c_addr_table[i];
		do {
			*sensor_id = return_sensor_id(ctx);
			if (*sensor_id == imgsensor_info.sensor_id) {
				LOG_ERR("i2c write id: 0x%x, sensor id: 0x%x\n",
					ctx->i2c_write_id, *sensor_id);

				if (1 == mot_ov32b_fusion_talk_en)
					ov32b_fusion_talk_read(ctx);

				return ERROR_NONE;
			}
			LOG_ERR("Read sensor id fail, id: 0x%x\n",
				ctx->i2c_write_id);
			retry--;
		} while (retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;
}

/*************************************************************************
 * FUNCTION
 *	open
 *
 * DESCRIPTION
 *	This function initialize the registers of CMOS sensor
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 ************************************************************************
 */
static int open(struct subdrv_ctx *ctx)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint16 sensor_id = 0;
	LOG_INF("open \n");
	// set_normal_mode(ctx);
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		ctx->i2c_write_id = imgsensor_info.i2c_addr_table[i];
		do {
			sensor_id = return_sensor_id(ctx);
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_ERR("i2c write id: 0x%x, sensor id: 0x%x\n",
					ctx->i2c_write_id, sensor_id);
				break;
			}
			LOG_ERR("Read sensor id fail, id: 0x%x\n", sensor_id);
			retry--;
		} while (retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id)
			break;
		retry = 2;
	}
	if (imgsensor_info.sensor_id != sensor_id)
		return ERROR_SENSOR_CONNECT_FAIL;

	/* initail sequence write in  */
	sensor_init(ctx);

	ctx->autoflicker_en = KAL_FALSE;
	ctx->sensor_mode = IMGSENSOR_MODE_INIT;
	ctx->shutter = 0x3D0;
	ctx->gain = BASEGAIN * 4;
	ctx->pclk = imgsensor_info.pre.pclk;
	ctx->frame_length = imgsensor_info.pre.framelength;
	ctx->line_length = imgsensor_info.pre.linelength;
	ctx->min_frame_length = imgsensor_info.pre.framelength;
	ctx->dummy_pixel = 0;
	ctx->dummy_line = 0;
	ctx->ihdr_mode = 0;
	ctx->test_pattern = KAL_FALSE;
	ctx->current_fps = imgsensor_info.pre.max_framerate;

	return ERROR_NONE;
} /*	open  */

/*************************************************************************
 * FUNCTION
 *	close
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static int close(struct subdrv_ctx *ctx)
{
	streaming_control(ctx, KAL_FALSE);

	LOG_INF("E\n");
	return ERROR_NONE;
} /*	close  */

/*************************************************************************
 * FUNCTION
 * preview
 *
 * DESCRIPTION
 *	This function start the sensor preview.
 *
 * PARAMETERS
 *	*image_window : address pointer of pixel numbers in one period of HSYNC
 *  *sensor_config_data : address pointer of line numbers in one period of VSYNC
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 preview(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_PREVIEW;
	ctx->pclk = imgsensor_info.pre.pclk;
	ctx->line_length = imgsensor_info.pre.linelength;
	ctx->frame_length = imgsensor_info.pre.framelength;
	ctx->min_frame_length = imgsensor_info.pre.framelength;
	ctx->current_fps = imgsensor_info.pre.max_framerate;
	ctx->autoflicker_en = KAL_FALSE;
	preview_setting(ctx);
	return ERROR_NONE;
} /*	preview   */

/*************************************************************************
 * FUNCTION
 *	capture
 *
 * DESCRIPTION
 *	This function setup the CMOS sensor in capture MY_OUTPUT mode
 *
 * PARAMETERS
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 capture(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_CAPTURE;
	ctx->pclk = imgsensor_info.cap.pclk;
	ctx->line_length = imgsensor_info.cap.linelength;
	ctx->frame_length = imgsensor_info.cap.framelength;
	ctx->min_frame_length = imgsensor_info.cap.framelength;
	ctx->current_fps = imgsensor_info.cap.max_framerate;
	ctx->autoflicker_en = KAL_FALSE;
	capture_setting(ctx);
	return ERROR_NONE;
} /* capture(ctx) */

static kal_uint32 normal_video(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			       MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_VIDEO;
	ctx->pclk = imgsensor_info.normal_video.pclk;
	ctx->line_length = imgsensor_info.normal_video.linelength;
	ctx->frame_length = imgsensor_info.normal_video.framelength;
	ctx->min_frame_length = imgsensor_info.normal_video.framelength;
	ctx->current_fps = imgsensor_info.normal_video.max_framerate;
	ctx->autoflicker_en = KAL_FALSE;
	video_setting(ctx);
	return ERROR_NONE;
} /*	normal_video   */

static kal_uint32 hs_video(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	ctx->pclk = imgsensor_info.hs_video.pclk;
	ctx->line_length = imgsensor_info.hs_video.linelength;
	ctx->frame_length = imgsensor_info.hs_video.framelength;
	ctx->min_frame_length = imgsensor_info.hs_video.framelength;
	ctx->current_fps = imgsensor_info.hs_video.max_framerate;
	ctx->autoflicker_en = KAL_FALSE;
	hs_video_setting(ctx);
	return ERROR_NONE;
} /*	hs_video   */

static kal_uint32 slim_video(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			     MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	ctx->pclk = imgsensor_info.slim_video.pclk;
	ctx->line_length = imgsensor_info.slim_video.linelength;
	ctx->frame_length = imgsensor_info.slim_video.framelength;
	ctx->min_frame_length = imgsensor_info.slim_video.framelength;
	ctx->current_fps = imgsensor_info.slim_video.max_framerate;
	ctx->autoflicker_en = KAL_FALSE;
	slim_video_setting(ctx);
	return ERROR_NONE;
} /*	slim_video	 */

static kal_uint32 custom1(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		       MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_CUSTOM1;
	ctx->pclk = imgsensor_info.custom1.pclk;
	ctx->line_length = imgsensor_info.custom1.linelength;
	ctx->frame_length = imgsensor_info.custom1.framelength;
	ctx->min_frame_length = imgsensor_info.custom1.framelength;
	ctx->current_fps = imgsensor_info.custom1.max_framerate;
	ctx->autoflicker_en = KAL_FALSE;
	custom1_setting(ctx);
	return ERROR_NONE;
}	/*	custom1   */

static int get_resolution(struct subdrv_ctx *ctx,
			  MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	int i = 0;

	for (i = SENSOR_SCENARIO_ID_MIN; i < SENSOR_SCENARIO_ID_MAX; i++) {
		if (i < imgsensor_info.sensor_mode_num) {
			sensor_resolution->SensorWidth[i] = imgsensor_winsize_info[i].w2_tg_size;
			sensor_resolution->SensorHeight[i] = imgsensor_winsize_info[i].h2_tg_size;
		} else {
			sensor_resolution->SensorWidth[i] = 0;
			sensor_resolution->SensorHeight[i] = 0;
		}
	}

	return ERROR_NONE;
} /* get_resolution */

static int get_info(struct subdrv_ctx *ctx, enum MSDK_SCENARIO_ID_ENUM scenario_id,
		    MSDK_SENSOR_INFO_STRUCT *sensor_info, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("scenario_id = %d\n", scenario_id);
	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
	/* not use */
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	/* inverse with datasheet*/
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4; /* not use */
	sensor_info->SensorResetActiveHigh = FALSE; /* not use */
	sensor_info->SensorResetDelayCount = 5; /* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	sensor_info->SensorOutputDataFormat =
		imgsensor_info.sensor_output_dataformat;

	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_NORMAL_PREVIEW] = imgsensor_info.pre_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_NORMAL_CAPTURE] = imgsensor_info.cap_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_NORMAL_VIDEO] = imgsensor_info.video_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO] = imgsensor_info.hs_video_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_SLIM_VIDEO] = imgsensor_info.slim_video_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_CUSTOM1] = imgsensor_info.custom1_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0;
	/* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;
	/* The frame of setting shutter default 0 for TG int */
	sensor_info->AESensorGainDelayFrame =
		imgsensor_info.ae_sensor_gain_delay_frame;
	/* The frame of setting sensor gain */
	sensor_info->AEISPGainDelayFrame =
		imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
	sensor_info->PDAF_Support = 0;
	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3; /* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2; /* not use */
	sensor_info->SensorPixelClockCount = 3; /* not use */
	sensor_info->SensorDataLatchCount = 2; /* not use */

	sensor_info->SensorWidthSampling = 0; /* 0 is default 1x*/
	sensor_info->SensorHightSampling = 0; /* 0 is default 1x*/
	sensor_info->SensorPacketECCOrder = 1;

	return ERROR_NONE;
} /*	get_info  */

static int control(struct subdrv_ctx *ctx, enum MSDK_SCENARIO_ID_ENUM scenario_id,
		   MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("666666   scenario_id = %d\n", scenario_id);
	ctx->current_scenario_id = scenario_id;
	switch (scenario_id) {
	case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		preview(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		capture(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		normal_video(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		hs_video(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		slim_video(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_CUSTOM1:
		custom1(ctx, image_window, sensor_config_data);
		break;
	default:
		LOG_INF_N("Error ScenarioId setting, use default mode");
		preview(ctx, image_window, sensor_config_data);
		return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
} /* control(ctx) */

static kal_uint32 set_video_mode(struct subdrv_ctx *ctx, UINT16 framerate)
{
	LOG_INF("framerate = %d\n ", framerate);
	if (framerate == 0) {
		/* Dynamic frame rate*/
		return ERROR_NONE;
	}
	if ((framerate == 300) && (ctx->autoflicker_en == KAL_TRUE))
		ctx->current_fps = 296;
	else if ((framerate == 150) && (ctx->autoflicker_en == KAL_TRUE))
		ctx->current_fps = 146;
	else
		ctx->current_fps = framerate;
	set_max_framerate(ctx, ctx->current_fps, 1);
	set_dummy(ctx);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(struct subdrv_ctx *ctx, kal_bool enable, UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d\n", enable, framerate);
	if (enable) { /*enable auto flicker*/
		ctx->autoflicker_en = KAL_TRUE;
	} else { /*Cancel Auto flick*/
		ctx->autoflicker_en = KAL_FALSE;
	}
	return ERROR_NONE;
}

static kal_uint32 set_max_framerate_by_scenario(struct subdrv_ctx *ctx, enum MSDK_SCENARIO_ID_ENUM scenario_id,
			      MUINT32 framerate)
{
	kal_uint32 frame_length;

	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
	case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
		ctx->dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length -
				 imgsensor_info.pre.framelength) : 0;
		ctx->frame_length = imgsensor_info.pre.framelength + ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		if (framerate == 0)
			return ERROR_NONE;
		frame_length = imgsensor_info.normal_video.pclk / framerate *
			       10 / imgsensor_info.normal_video.linelength;
		ctx->dummy_line = (frame_length > imgsensor_info.normal_video.framelength) ?
				      (frame_length - imgsensor_info.normal_video.framelength) : 0;
		ctx->frame_length = imgsensor_info.normal_video.framelength + ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		if (ctx->current_fps != imgsensor_info.cap.max_framerate) {
			LOG_INF("Warning: current_fps %d fps is not support",
				framerate);
			LOG_INF("so use cap's setting: %d fps!\n",
				imgsensor_info.cap.max_framerate / 10);
		}
		frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
		ctx->dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length -
				 imgsensor_info.cap.framelength) : 0;
		ctx->frame_length = imgsensor_info.cap.framelength + ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
		ctx->dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length -
				 imgsensor_info.hs_video.framelength) : 0;
		ctx->frame_length = imgsensor_info.hs_video.framelength + ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
		ctx->dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length -
				 imgsensor_info.slim_video.framelength) : 0;
		ctx->frame_length = imgsensor_info.slim_video.framelength + ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_CUSTOM1:
		frame_length = imgsensor_info.custom1.pclk / framerate * 10 / imgsensor_info.custom1.linelength;
		ctx->dummy_line = (frame_length > imgsensor_info.custom1.framelength) ? (frame_length -
         imgsensor_info.custom1.framelength) : 0;
		ctx->frame_length = imgsensor_info.custom1.framelength + ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	default: /*coding with  preview scenario by default*/
		frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
		ctx->dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length -
				 imgsensor_info.pre.framelength) : 0;
		ctx->frame_length = imgsensor_info.pre.framelength + ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		LOG_INF("error scenario_id = %d, we use preview scenario\n",
			scenario_id);
		break;
	}
	return ERROR_NONE;
}

static kal_uint32 get_default_framerate_by_scenario(struct subdrv_ctx *ctx, enum MSDK_SCENARIO_ID_ENUM scenario_id,
				  MUINT32 *framerate)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	switch (scenario_id) {
	case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		*framerate = imgsensor_info.pre.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		*framerate = imgsensor_info.normal_video.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		*framerate = imgsensor_info.cap.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		*framerate = imgsensor_info.hs_video.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		*framerate = imgsensor_info.slim_video.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_CUSTOM1:
		*framerate = imgsensor_info.custom1.max_framerate;
		break;
	default:
		break;
	}
	return ERROR_NONE;
}
static kal_uint32 set_test_pattern_mode(struct subdrv_ctx *ctx, kal_bool enable)
{
	LOG_INF("Test_Pattern enable: %d\n", enable);
	if (enable) {
		write_cmos_sensor_8(ctx,0x3208,0x01);
		write_cmos_sensor_8(ctx,0x50c1,0x01);
		write_cmos_sensor_8(ctx,0x50c2,0xff);
		write_cmos_sensor_8(ctx,0x3208,0x11);
		write_cmos_sensor_8(ctx,0x3208,0xa1);
	} else {
		write_cmos_sensor_8(ctx,0x3208,0x01);
		write_cmos_sensor_8(ctx,0x50c1,0x00);
		write_cmos_sensor_8(ctx,0x50c2,0x00);
		write_cmos_sensor_8(ctx,0x3208,0x11);
		write_cmos_sensor_8(ctx,0x3208,0xa1);
	}
	ctx->test_pattern = enable;
	return ERROR_NONE;
}

static int feature_control(struct subdrv_ctx *ctx, MSDK_SENSOR_FEATURE_ENUM feature_id,
			   UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *)feature_para;
	UINT16 *feature_data_16 = (UINT16 *)feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *)feature_para;
	UINT32 *feature_data_32 = (UINT32 *)feature_para;
	//    INT32 *feature_return_para_i32 = (INT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *)feature_para;
	//	char *data = (char *)(uintptr_t)(*(feature_data + 1));
	//	UINT16 type = (UINT16)(*feature_data);

	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;

	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
		(MSDK_SENSOR_REG_INFO_STRUCT *)feature_para;

	// LOG_INF("feature_id = %d\n", feature_id);
	switch (feature_id) {
	case SENSOR_FEATURE_GET_OUTPUT_FORMAT_BY_SCENARIO:
		switch (*feature_data) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		case SENSOR_SCENARIO_ID_CUSTOM1:
			*(feature_data + 1) = (enum ACDK_SENSOR_OUTPUT_DATA_FORMAT_ENUM)
					imgsensor_info.sensor_output_dataformat;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_ANA_GAIN_TABLE:
		if ((void *)(uintptr_t)(*(feature_data + 1)) == NULL) {
			*(feature_data + 0) = sizeof(ov32b_ana_gain_table);
		} else {
			memcpy((void *)(uintptr_t)(*(feature_data + 1)), (void *)ov32b_ana_gain_table,
			       sizeof(ov32b_ana_gain_table));
		}
		break;
	case SENSOR_FEATURE_GET_GAIN_RANGE_BY_SCENARIO:
		*(feature_data + 1) = imgsensor_info.min_gain;
		*(feature_data + 2) = imgsensor_info.max_gain;
		break;
	case SENSOR_FEATURE_GET_BASE_GAIN_ISO_AND_STEP:
		*(feature_data + 0) = imgsensor_info.min_gain_iso;
		*(feature_data + 1) = imgsensor_info.gain_step;
		*(feature_data + 2) = imgsensor_info.gain_type;
		break;
	case SENSOR_FEATURE_GET_MAX_EXP_LINE:
		*(feature_data + 2) =
			imgsensor_info.max_frame_length - imgsensor_info.margin;
	case SENSOR_FEATURE_GET_MIN_SHUTTER_BY_SCENARIO:
		*(feature_data + 1) = imgsensor_info.min_shutter;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ_BY_SCENARIO:
		switch (*feature_data) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = imgsensor_info.cap.pclk;
			break;
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = imgsensor_info.normal_video.pclk;
			break;
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = imgsensor_info.hs_video.pclk;
			break;
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = imgsensor_info.slim_video.pclk;
			break;
		case SENSOR_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = imgsensor_info.custom1.pclk;
			break;
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = imgsensor_info.pre.pclk;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_OFFSET_TO_START_OF_EXPOSURE:
		*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 2500000;
		break;
	case SENSOR_FEATURE_GET_PERIOD_BY_SCENARIO:
		switch (*feature_data) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = (imgsensor_info.cap.framelength <<
        16) + imgsensor_info.cap.linelength;
			break;
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = (imgsensor_info.normal_video.framelength <<
        16) + imgsensor_info.normal_video.linelength;
			break;
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = (imgsensor_info.hs_video.framelength <<
        16) + imgsensor_info.hs_video.linelength;
			break;
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = (imgsensor_info.slim_video.framelength <<
        16) + imgsensor_info.slim_video.linelength;
			break;
		case SENSOR_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = (imgsensor_info.custom1.framelength <<
        16) + imgsensor_info.custom1.linelength;
			break;
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = (imgsensor_info.pre.framelength <<
        16) + imgsensor_info.pre.linelength;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PERIOD:
		*feature_return_para_16++ = ctx->line_length;
		*feature_return_para_16 = ctx->frame_length;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
		*feature_return_para_32 = ctx->pclk;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_ESHUTTER:
		set_shutter(ctx, *feature_data);
		break;
	case SENSOR_FEATURE_SET_NIGHTMODE:
		night_mode(ctx, (BOOL)*feature_data);
		break;
	case SENSOR_FEATURE_SET_GAIN:
		set_gain(ctx, (UINT32)*feature_data);
		break;
	case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		write_cmos_sensor(ctx, sensor_reg_data->RegAddr, sensor_reg_data->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		sensor_reg_data->RegData = read_cmos_sensor(ctx, sensor_reg_data->RegAddr);
		break;
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		/* get the lens driver ID from EEPROM */
		/* or just return LENS_DRIVER_ID_DO_NOT_CARE */
		/* if EEPROM does not exist in camera module.*/
		*feature_return_para_32 = LENS_DRIVER_ID_DO_NOT_CARE;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_VIDEO_MODE:
		set_video_mode(ctx, *feature_data);
		break;
	case SENSOR_FEATURE_CHECK_SENSOR_ID:
		get_imgsensor_id(ctx, feature_return_para_32);
		break;
	case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
		set_auto_flicker_mode(ctx, (BOOL)*feature_data_16, *(feature_data_16 + 1));
		break;
	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		set_max_framerate_by_scenario(ctx, (enum MSDK_SCENARIO_ID_ENUM) * feature_data,
			*(feature_data + 1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario(ctx, (enum MSDK_SCENARIO_ID_ENUM) * (feature_data),
			(MUINT32 *)(uintptr_t)(*(feature_data + 1)));
		break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode(ctx, (BOOL)*feature_data);
		break;
	/*for factory mode auto testing*/
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_FRAMERATE:
		ctx->current_fps = *feature_data_16;
		break;
	case SENSOR_FEATURE_GET_CROP_INFO:
		LOG_INF("SENSOR_FEATURE_GET_CROP_INFO:");
		LOG_INF("scenarioId:%d\n", *feature_data_32);
		wininfo = (struct SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)(*(feature_data + 1));
		switch (*feature_data_32) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[1],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[2],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[3],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[4],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_CUSTOM1:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[5],
				    sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		default:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[0],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		}
		break;
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		set_shutter_frame_length(ctx, (UINT32)*feature_data, (UINT32) * (feature_data + 1));
		break;
#if 0
	case SENSOR_FEATURE_GET_4CELL_DATA:
		/*get 4 cell data from eeprom*/
		if (type == FOUR_CELL_CAL_TYPE_XTALK_CAL) {
			LOG_INF("Read Cross Talk Start");
			read_four_cell_from_eeprom(ctx, data);
			LOG_INF("Read Cross Talk = %02x %02x %02x %02x %02x %02x\n",
				(UINT16)data[0], (UINT16)data[1],
				(UINT16)data[2], (UINT16)data[3],
				(UINT16)data[4], (UINT16)data[5]);
		}
		break;
#endif

	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(ctx, KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_RESUME");
		LOG_INF("shutter:%llu\n", *feature_data);
		if (*feature_data != 0)
			set_shutter(ctx, *feature_data);
		streaming_control(ctx, KAL_TRUE);
		break;
	case SENSOR_FEATURE_GET_BINNING_TYPE:
		switch (*(feature_data + 1)) {
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		case SENSOR_SCENARIO_ID_CUSTOM1:
		default:
			*feature_return_para_32 = 1000;
			break;
		}
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE: {
		kal_uint32 rate;

		switch (*feature_data) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
			rate = imgsensor_info.cap.mipi_pixel_rate;
			break;
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
			rate = imgsensor_info.normal_video.mipi_pixel_rate;
			break;
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
			rate = imgsensor_info.hs_video.mipi_pixel_rate;
			break;
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
			rate = imgsensor_info.slim_video.mipi_pixel_rate;
			break;
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
			rate = imgsensor_info.pre.mipi_pixel_rate;
			break;
		case SENSOR_SCENARIO_ID_CUSTOM1:
			rate = imgsensor_info.custom1.mipi_pixel_rate;
			break;
		default:
			rate = 0;
			break;
		}
		*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = rate;
	} break;

#if 0
	case SENSOR_FEATURE_PRELOAD_EEPROM_DATA:
		/*get eeprom preloader data*/
		*feature_return_para_32 = ctx->is_read_four_cell;
		*feature_para_len = 4;
		if (ctx->is_read_four_cell != 1)
			read_four_cell_from_eeprom(ctx, NULL);
		break;
#endif
	case SENSOR_FEATURE_SET_FRAMELENGTH:
		set_frame_length(ctx, (UINT32)(*feature_data));
		break;

	default:
		break;
	}
	return ERROR_NONE;
} /*	feature_control(ctx)  */
#ifdef IMGSENSOR_VC_ROUTING
static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0xCC0,
			.vsize = 0x990,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cap[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0xCC0,
			.vsize = 0x990,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0xCC0,
			.vsize = 0x990,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_slim_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0xCC0,
			.vsize = 0x990,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus1[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0xCC0,
			.vsize = 0x990,
		},
	},
};

#endif
static int get_frame_desc(struct subdrv_ctx *ctx, int scenario_id, struct mtk_mbus_frame_desc *fd)
{
	switch (scenario_id) {
	case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		fd->type = MTK_MBUS_FRAME_DESC_TYPE_CSI2;
		fd->num_entries = ARRAY_SIZE(frame_desc_prev);
		memcpy(fd->entry, frame_desc_prev, sizeof(frame_desc_prev));
		break;
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		fd->type = MTK_MBUS_FRAME_DESC_TYPE_CSI2;
		fd->num_entries = ARRAY_SIZE(frame_desc_cap);
		memcpy(fd->entry, frame_desc_cap, sizeof(frame_desc_cap));
		break;
	case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		fd->type = MTK_MBUS_FRAME_DESC_TYPE_CSI2;
		fd->num_entries = ARRAY_SIZE(frame_desc_vid);
		memcpy(fd->entry, frame_desc_vid, sizeof(frame_desc_vid));
		break;
	case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		fd->type = MTK_MBUS_FRAME_DESC_TYPE_CSI2;
		fd->num_entries = ARRAY_SIZE(frame_desc_slim_vid);
		memcpy(fd->entry, frame_desc_slim_vid, sizeof(frame_desc_slim_vid));
		break;
	case SENSOR_SCENARIO_ID_CUSTOM1:
		fd->type = MTK_MBUS_FRAME_DESC_TYPE_CSI2;
		fd->num_entries = ARRAY_SIZE(frame_desc_cus1);
		memcpy(fd->entry, frame_desc_cus1, sizeof(frame_desc_cus1));
		break;
	default:
		return -1;
	}
	return 0;
}

static const struct subdrv_ctx defctx = {

	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_max = BASEGAIN * 15.9375, //1024 * 15.9375=16320
	.ana_gain_min = BASEGAIN,
	.ana_gain_step = 32,
	.exposure_def = 0x3D0,
	.exposure_max = 0xffff - 31,
	.exposure_min = 16,
	.exposure_step = 1,
	.margin = 31,
	.max_frame_length = 0xffff,

	.mirror = IMAGE_NORMAL, //mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x03D0, /*current shutter*/
	.gain = BASEGAIN * 4, /*current gain*/
	.dummy_pixel = 0, /*current dummypixel*/
	.dummy_line = 0, /*current dummyline*/
	.current_fps = 300,
	/*full size current fps : 24fps for PIP, 30fps for Normal or ZSD*/
	.autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,

	.current_scenario_id = SENSOR_SCENARIO_ID_NORMAL_PREVIEW,
	.ihdr_mode = 0, /*sensor need support LE, SE with HDR feature*/
	.i2c_write_id = 0x20,
};

static int init_ctx(struct subdrv_ctx *ctx, struct i2c_client *i2c_client, u8 i2c_write_id)
{
	memcpy(ctx, &defctx, sizeof(*ctx));
	ctx->i2c_client = i2c_client;
	ctx->i2c_write_id = i2c_write_id;
	return 0;
}

static struct subdrv_ops ops = {
	.get_id = get_imgsensor_id,
	.init_ctx = init_ctx,
	.open = open,
	.get_info = get_info,
	.get_resolution = get_resolution,
	.control = control,
	.feature_control = feature_control,
	.close = close,
#ifdef IMGSENSOR_VC_ROUTING
	.get_frame_desc = get_frame_desc,
#endif
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{ HW_ID_RST, 0, 1 },
	{ HW_ID_MCLK, 24, 0 },
	{ HW_ID_MCLK_DRIVING_CURRENT, 6, 1 },
	{ HW_ID_DOVDD, 1800000, 1 },
	{ HW_ID_AVDD, 2800000, 1 },
	{ HW_ID_DVDD, 1100000, 1 },
	{ HW_ID_RST, 1, 2 },
};

const struct subdrv_entry mot_manaus_ov32b_mipi_raw_entry = {
	.name = SENSOR_DRVNAME_MOT_MANAUS_OV32B_MIPI_RAW,
	.id = MOT_MANAUS_OV32B_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
};
