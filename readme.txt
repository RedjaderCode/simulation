NEXT THINGS TO DO:

 - render pipeline is causing a bottleneck. Implement different pipeline.
 - create a threadpool or create another std::thread to split the raymarching calculations between portions of the screenwidth
 - perhaps it's time to retry d3dx11...
 - Further back-end cell matrix optimizations. at 60 fps target framerate, cpu runs at about 75%