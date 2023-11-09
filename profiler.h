#ifndef PROFILER_H
#define PROFILER_H

#include <chrono>
#include <string>
#include <iostream>

struct cProfiler
{
	std::string m_Name;
	std::chrono::high_resolution_clock::time_point m_Start;
	__int64 m_Threshold;
	bool m_AutoRecord;
	using dura = std::chrono::milliseconds;
	cProfiler(const std::string& Name, __int64 Threshold = 0, bool AutoRecord = true) :
		m_Name(Name), m_Threshold(Threshold), m_AutoRecord(AutoRecord), m_Start(std::chrono::high_resolution_clock::now())
	{
	}
	~cProfiler()
	{
		if (m_AutoRecord == false)
			return;

		auto ElapsedTime = GetElapsedTime();
		if (m_Threshold == 0)
		{
			std::cout << m_Name << ": " << ElapsedTime << std::endl;
		}
		else if (ElapsedTime >= m_Threshold)
		{
			std::cout << m_Name << ": " << ElapsedTime << std::endl;
		}
	}

	void Set()
	{
		auto ElapsedTime = GetElapsedTime();
		if (m_Threshold == 0)
		{
			std::cout << m_Name << ": " << ElapsedTime << std::endl;
		}
		else if (ElapsedTime >= m_Threshold)
		{
			std::cout << m_Name << ": " << ElapsedTime << std::endl;
		}
		m_Start = std::chrono::high_resolution_clock::now();

	}
	__int64 GetElapsedTime() {
		dura d = std::chrono::duration_cast<dura>(std::chrono::high_resolution_clock::now() - m_Start);
		return d.count();
	}
};

#endif