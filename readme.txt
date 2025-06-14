6/14 - log

After my main source file was for some reason deleted, I ragained an older version. NOTE:

 - use barriers passed to each thread to sync them together.
	auto function = [](){}; //function which is run after all threads sync
	std::barrier var(numOfThreads, function)

	thread function -> required template<class C> -> parameter requires -> std::barrier<C> var

 - malloc fixes are required...

 - thread throttling using chrono and sleep_for()