////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>

#include <mmap-manager.h>

#include <cstring>

#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST (MemoryMapManager, ParseAndroidDeviceMaps32bit)
{
  //
  // 32-bit /proc/<pid>/maps data.
  //

  const char *dataBuffer = "\
a111a000-a111b000 ---p 00000000 00:00 0 \n\
13201000-22c00000 ---p 00601000 00:04 159        /dev/ashmem/dalvik-main space (deleted)\n\
6f638000-70153000 rw-p 00000000 08:13 40965      /data/dalvik-cache/x86/system@framework@boot.art\n\
a111c000-a1218000 rw-p 00000000 00:00 0          [stack:2262]\n\
a2a00000-a3400000 rw-p 00000000 00:00 0          [anon:libc_malloc]\n\
a3532000-a3735000 r--p 00000000 08:13 41136      /data/dalvik-cache/x86/data@app@ApiDemos@ApiDemos.apk@classes.dex\n\
a3f9f000-a3fad000 r-xp 00000000 08:06 711        /system/lib/egl/libGLESv2_emulation.so\n\
b0202000-b023d000 r--p 00000000 08:06 613        /system/fonts/Roboto-Regular.ttf\n\
b36f7000-b3701000 r--s 004fc000 08:13 14         /data/app/ApiDemos/ApiDemos.apk\n\
b4888000-b48a8000 r--s 00000000 00:0c 8201       /dev/__properties__\n\
b7783000-b7786000 r-xp 00000000 08:06 143        /system/bin/app_process32\n\
";

  const char *filename = "android-proc-maps-32bit.txt";

  FILE *mapsFile = fopen (filename, "w");

  ASSERT_NE (mapsFile, (FILE*)NULL);

  if (mapsFile)
  {
    size_t dataSize = strlen (dataBuffer);

    ASSERT_GT (dataSize, size_t (0));

    ASSERT_EQ (fwrite (dataBuffer, 1, dataSize, mapsFile), dataSize);

    ASSERT_EQ (fflush (mapsFile), 0);

    ASSERT_EQ (fclose (mapsFile), 0);
  }

  MemoryMapManager manager;

  size_t processedMaps = manager.ParseUnixProcessMapsFile (filename);

  ASSERT_EQ (processedMaps, size_t (9));

  remove (filename);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST (MemoryMapManager, ParseAndroidDeviceMaps64bit)
{
  //
  // 64-bit /proc/<pid>/maps data.
  //

  const char *dataBuffer = "\
a111a000a111a000-a111b000a111b000 ---p 0000000000000000 00:00 0 \n\
1320100013201000-22c0000022c00000 ---p 0060100000601000 00:04 159        /dev/ashmem/dalvik-main space (deleted)\n\
6f6380006f638000-7015300070153000 rw-p 0000000000000000 08:13 40965      /data/dalvik-cache/x86/system@framework@boot.art\n\
a111c000a111c000-a1218000a1218000 rw-p 0000000000000000 00:00 0          [stack:2262]\n\
a2a00000a2a00000-a3400000a3400000 rw-p 0000000000000000 00:00 0          [anon:libc_malloc]\n\
a3532000a3532000-a3735000a3735000 r--p 0000000000000000 08:13 41136      /data/dalvik-cache/x86/data@app@ApiDemos@ApiDemos.apk@classes.dex\n\
a3f9f000a3f9f000-a3fad000a3fad000 r-xp 0000000000000000 08:06 711        /system/lib/egl/libGLESv2_emulation.so\n\
b0202000b0202000-b023d000b023d000 r--p 0000000000000000 08:06 613        /system/fonts/Roboto-Regular.ttf\n\
b36f7000b36f7000-b3701000b3701000 r--s 004fc000004fc000 08:13 14         /data/app/ApiDemos/ApiDemos.apk\n\
b4888000b4888000-b48a8000b48a8000 r--s 0000000000000000 00:0c 8201       /dev/__properties__\n\
b7783000b7783000-b7786000b7786000 r-xp 0000000000000000 08:06 143        /system/bin/app_process32\n\
";

  const char *filename = "android-proc-maps-64bit.txt";

  FILE *mapsFile = fopen (filename, "w");

  ASSERT_NE (mapsFile, (FILE*)NULL);

  if (mapsFile)
  {
    size_t dataSize = strlen (dataBuffer);

    ASSERT_GT (dataSize, size_t (0));

    ASSERT_EQ (fwrite (dataBuffer, 1, dataSize, mapsFile), dataSize);

    ASSERT_EQ (fflush (mapsFile), 0);

    ASSERT_EQ (fclose (mapsFile), 0);
  }

  MemoryMapManager manager;

  size_t processedMaps = manager.ParseUnixProcessMapsFile (filename);

  ASSERT_EQ (processedMaps, size_t (9));

  remove (filename);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST (MemoryMapManager, PopulateJsonObject)
{
  MemoryMapManager manager;

  kvr::ctx *jsonContext = kvr::ctx::create ();

  ASSERT_NE (jsonContext, (kvr::ctx*)NULL);

  JsonNodeKvr jsonRootNode (jsonContext->create_value ()->conv_map ());

  ASSERT_TRUE (manager.PopulateJsonObject (jsonRootNode));

  kvr::ctx::destroy (jsonContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main (int argc, char *argv [])
{
  testing::InitGoogleTest (&argc, argv);

  return RUN_ALL_TESTS ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
