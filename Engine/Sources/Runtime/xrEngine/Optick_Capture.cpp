///////////////////////////////////////////////////////////////////////////////////
// Created: 05.01.2025
// Author: NSDeathman
// Nocturning studio for NS Platform X
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop

#include "igame_level.h"
#include "igame_persistent.h"

#include "Optick_Capture.h"
///////////////////////////////////////////////////////////////////////////////////
ENGINE_API COptickCapture OptickCapture;
///////////////////////////////////////////////////////////////////////////////////
void COptickCapture::Initialize()
{
	Msg("\n");
	Msg("Initializing OptickCapture");
	m_frames_to_capture = 1;
	m_start_capture_frame = 1;
	m_end_capture_frame = 1;
	m_need_capture = false;
	m_switched_to_capturing = false;
	OPTICK_STOP_CAPTURE();
};

void COptickCapture::Destroy()
{
	if(m_need_capture)
		StopCapturing();
};

/*********************************************************************************
Get saves path
*********************************************************************************/
xr_string COptickCapture::GetSavePath() const
{
	string_path capture_path;
	FS.update_path(capture_path, "$app_data_root$", "");

	strconcat(sizeof(capture_path), capture_path, capture_path, "optick_captures\\");

	FS.create_dir(capture_path);

	return capture_path;
}

/*********************************************************************************
Base methods: Start, Stop, Save
*********************************************************************************/
void COptickCapture::StartCapturing()
{
	OPTICK_START_CAPTURE(Optick::Mode::Type(Optick::Mode::INSTRUMENTATION |
											Optick::Mode::TAGS |
											Optick::Mode::IO));
	Msg("- Optick capturing started");
}

/* Saving in try-statement for avoiding game crash */
void COptickCapture::TryToSaveCapture(str_c save_path)
{
	try
	{
		OPTICK_SAVE_CAPTURE(save_path);
	}
	catch(...)
	{
		Msg("! An error occurred while saving optick capture");
	}
};

void COptickCapture::SaveCapture(str_c save_path)
{
	TryToSaveCapture(save_path);
	Msg("- Optick capture saved with name: %s", save_path);
};

/*********************************************************************************
Capturing a specified number of frames.
*********************************************************************************/
void COptickCapture::OnFrame()
{
	if(m_need_capture)
	{
		if(Engine.TimeManager.GetFrameCount() == m_end_capture_frame)
		{
			StopCapturing();
			SaveCapturedFrames();
		}
	}
};

void COptickCapture::StartCapturing(int frames_to_capture)
{
	m_need_capture = true;
	m_frames_to_capture = frames_to_capture;
	m_start_capture_frame = Engine.TimeManager.GetFrameCount();
	m_end_capture_frame = m_start_capture_frame + m_frames_to_capture;

	StartCapturing();
};

void COptickCapture::StopCapturing()
{
	OPTICK_STOP_CAPTURE();
	m_need_capture = false;

	Msg("- Optick capturing stoped");
};

void COptickCapture::SaveCapturedFrames()
{
	Msg("- Saving captured frames");

	LPCSTR frames = (m_frames_to_capture == 1) ? "frame" : "frames";

	xr_string capture_path = GetSavePath();

	string_path file_name;
	sprintf(file_name, "optick_capture_%d_%s.opt", m_frames_to_capture, frames);

	capture_path += file_name;

	SaveCapture(capture_path.c_str());
}

/*********************************************************************************
Capturing in switcher mode:
	first call switching to capture, second switching to save
*********************************************************************************/
void COptickCapture::SwitchProfiler()
{
	if(m_need_capture)
	{
		Msg("! Capturing already started, please wait until end of capturing and try again");
		return;
	}

	if(!m_switched_to_capturing)
	{
		Msg("- OptickAPI switched to capturing mode, execute command again to switch to saving mode");
		SwitchToCapturing();
		m_switched_to_capturing = true;
	}
	else
	{
		Msg("- OptickAPI switched to saving mode");
		SwitchToSaving();
		m_switched_to_capturing = false;
	}
};

void COptickCapture::SwitchToCapturing()
{
	m_start_capture_frame = Engine.TimeManager.GetFrameCount();

	StartCapturing();
};

void COptickCapture::SwitchToSaving()
{
	StopCapturing();

	m_frames_to_capture = Engine.TimeManager.GetFrameCount() - m_start_capture_frame;

	Msg("- Saving %d frames", m_frames_to_capture);

	LPCSTR frames = (m_frames_to_capture == 1) ? "frame" : "frames";

	xr_string capture_path = GetSavePath();

	string_path file_name;
	sprintf(file_name, "optick_capture_%d_%s.opt", m_frames_to_capture, frames);

	capture_path += file_name;

	SaveCapture(capture_path.c_str());
}
///////////////////////////////////////////////////////////////////////////////////
