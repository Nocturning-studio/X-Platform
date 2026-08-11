/*---------------------------------------------------------------------------------------------
* Code by OpenXray project
---------------------------------------------------------------------------------------------*/
#pragma once

class Noncopyable
{
  public:
	Noncopyable() = default;
	Noncopyable(const Noncopyable&) = delete;
	Noncopyable& operator=(const Noncopyable&) = delete;
};