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
};

void COptickCapture::Destroy()
{
	if (m_need_capture)
		StopCapturing();
};

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
	catch (...)
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
	if (m_need_capture)
	{
		if (Engine.TimeManager.GetFrameCount() == m_end_capture_frame)
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

	shared_str capture_path;
	LPCSTR frames = m_frames_to_capture == 1 ? "frame" : "frames";
	capture_path.sprintf("optick_capture_%d_%s.opt", m_frames_to_capture, frames);

	SaveCapture(capture_path.c_str());
};

/*********************************************************************************
Capturing in switcher mode: 
	first call switching to capture, second switching to save
*********************************************************************************/
void COptickCapture::SwitchProfiler()
{
	if (m_need_capture)
	{
		Msg("! Capturing already started, please wait until end of capturing and try again");
		return;
	}

	if (!m_switched_to_capturing)
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

	shared_str capture_path;
	LPCSTR frames = m_frames_to_capture == 1 ? "frame" : "frames";
	capture_path.sprintf("optick_capture_%d_%s.opt", m_frames_to_capture, frames);

	SaveCapture(capture_path.c_str());
};
///////////////////////////////////////////////////////////////////////////////////
