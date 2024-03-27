#include "Thread.h"
#include "Globals.h"

std::unordered_map<std::string, Thread::ProcessFunction> Thread::m_mainThreadQueue;
std::mutex Thread::m_mainThreadQueueMutex;

void Thread::SubmitToMainThread(const std::string& functionName, ProcessFunction function)
{
	std::scoped_lock<std::mutex> lock(m_mainThreadQueueMutex);
	m_mainThreadQueue[functionName] = function;
}

void Thread::ExecuteMainThreadQueue()
{
	std::scoped_lock<std::mutex> lock(m_mainThreadQueueMutex);
	const auto processor = Processor::GetSingleton();

	for (const auto& func : m_mainThreadQueue)
	{
		//g_Logger->info("Function: {}", func.first.c_str());
		(processor->*func.second)(); // Run function
	}
	m_mainThreadQueue.clear();

}

void Thread::Run()
{
	if (m_mainThreadQueue.find("Weather") != m_mainThreadQueue.end())
	{
		//g_Logger->info("Attaching WeatherThread");
		ExecuteMainThreadQueue();
	}

	if (m_mainThreadQueue.find("Interior") != m_mainThreadQueue.end())
	{
		//g_Logger->info("Attaching InteriorThread");
		ExecuteMainThreadQueue();
	}

	if (m_mainThreadQueue.find("Time") != m_mainThreadQueue.end())
	{
		//g_Logger->info("Attaching TimeThread");
		ExecuteMainThreadQueue();
	}

	if (EnableMenus)
	{
		RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(Processor::GetSingleton());
	}
	else
	{
		RE::UI::GetSingleton()->RemoveEventSink<RE::MenuOpenCloseEvent>(Processor::GetSingleton());
	}

}

void Thread::RuntimeThread()
{
	SKSE::log::info("Attaching RuntimeThread");

	while (isLoaded)
	{

		if (EnableTime)
		{
			//g_Logger->info("Adding Time to Mainqueue");
			std::this_thread::sleep_for(std::chrono::seconds(TimeUpdateIntervalTime));
			SubmitToMainThread("Time", &Processor::ProcessTimeBasedToggling);
		}

		if (EnableInterior)
		{
			//g_Logger->info("Adding Interior to Mainqueue");
			std::this_thread::sleep_for(std::chrono::seconds(TimeUpdateIntervalInterior));
			SubmitToMainThread("Interior", &Processor::ProcessInteriorBasedToggling);
		}

		if (EnableWeather && !IsInInteriorCell)
		{
			//g_Logger->info("Adding Weather to Mainqueue");
			std::this_thread::sleep_for(std::chrono::seconds(TimeUpdateIntervalWeather));
			SubmitToMainThread("Weather", &Processor::ProcessWeatherBasedToggling);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		Run();
	}
}