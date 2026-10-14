////////////////////////////////////////////////////////////////////////////////
//  X-Platform — расширение функциональности X-Ray Engine
//
//  Автор:        NSDeathman
//  Студия:       Nocturning Studio
//
//  SPDX-FileCopyrightText: 2026 Nocturning Studio и участники X-Platform
//  SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
//
//  Этот файл распространяется на условиях PolyForm Noncommercial 1.0.0.
//  Копирование, изменение и распространение разрешены для некоммерческих
//  целей. Публичное указание авторства в вашем проекте, документации или
//  рекламе не требуется — оно приветствуется, но не является условием
//  использования.
//
//  При распространении копий или производных работ лицензия обязывает
//  сохранять настоящие уведомления и передавать получателю текст лицензии
//  или ссылку на него. Пожалуйста, не удаляйте эту плашку — это помогает
//  сохранять историю проекта и корректно соблюдать лицензию.
//
//  Коммерческое использование без отдельного разрешения правообладателей
//  запрещено.
//
//  Полный текст лицензии:
//      https://polyformproject.org/licenses/noncommercial/1.0.0
//
//  Этот файл является source-available, а не open source.
////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
class ENGINE_API COptickCapture
{
  private:
	u32 m_frames_to_capture;
	u32 m_start_capture_frame;
	u32 m_end_capture_frame;

	bool m_need_capture;
	bool m_switched_to_capturing;

  public:
	void Initialize();
	void Destroy();

	void OnFrame();
	void StartCapturing();
	void StartCapturing(int frames_to_capture);
	void StopCapturing();
	void SaveCapturedFrames();
	void TryToSaveCapture(str_c save_path);
	void SaveCapture(str_c save_path);

	void SwitchProfiler();
	void SwitchToCapturing();
	void SwitchToSaving();

	xr_string GetSavePath() const;
};
///////////////////////////////////////////////////////////////////////////////////
extern ENGINE_API COptickCapture OptickCapture;
///////////////////////////////////////////////////////////////////////////////////
