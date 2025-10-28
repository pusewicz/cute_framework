## Adding new Sample Code

The CF [samples](https://github.com/RandyGaul/cute_framework/tree/master/samples) are quite easy to extend. Simply copy + paste one of the other samples to get started, such as one of the simpler ones, perhaps [basic_sprite](https://github.com/RandyGaul/cute_framework/blob/master/samples/basic_sprite.cppb). or [basic_input.c](https://github.com/RandyGaul/cute_framework/blob/master/samples/basic_input.c).

Open up CF's `samples` [CmakeLists.txt file](https://github.com/RandyGaul/cute_framework/blob/master/samples/CMakeLists.txt) to hook up the sample to the build system. Add your new sample here like so for the example "new_sample":

```cmake
...
add_sample(waves waves.cpp)
add_sample(shallow_water shallow_water.cpp)
add_sample(noise noise.c)
add_sample(new_sample new_sample.c)
```

If your sample needs access to files on disk, such as assets like images or audio, create a folder in CF's samples folder. Name it "new_sample_data", where "new_sample" is the name of your new sample. Then add in a line to CF's [CmakeLists.txt file](https://github.com/RandyGaul/cute_framework/blob/master/samples/CMakeLists.txt) to copy over the assets to the build folder when building.

```cmake
	add_custom_command(TARGET spaceshooter PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/spaceshooter_data $<TARGET_FILE_DIR:spaceshooter>/spaceshooter_data)
	add_custom_command(TARGET waves PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/waves_data $<TARGET_FILE_DIR:waves>/waves_data)
	add_custom_command(TARGET shallow_water PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shallow_water_data $<TARGET_FILE_DIR:shallow_water>/shallow_water_data)
	add_custom_command(TARGET shallow_water PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/new_sample $<TARGET_FILE_DIR:shallow_water>/new_sample)
```

And that's it! Regenerate your project and you will be able to build your new sample. Once confirmed working as intended, open a pull request to add in your new sample!
