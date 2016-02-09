////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>

#include <mmap-manager.h>

#include <cstring>

#include <stdio.h>

#include <errno.h>

#include <assert.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
static constexpr size_t g_numTestEntries = 2;

static const char *g_mapsFileTestData [g_numTestEntries] =
{
// 
// 32-bit /proc/<pid>/maps data.
// 
"\
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
",
// 
// 64-bit /proc/<pid>/maps data.
// 
"\
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
"
};
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
int main (int argc, char* argv [])
{
  (void) argc;

  (void) argv;

  // 
  // Validate /proc/<pid>/maps parsing logic.
  // 

  for (size_t i = 0; i < g_numTestEntries; ++i)
  {
    char filename [64];

    sprintf (filename, "maps%zu.txt", i);

    FILE *mapsFile = fopen (filename, "w");

    const char *dataBuffer = g_mapsFileTestData[i];

    size_t dataSize = strlen (dataBuffer);

    if (fwrite (dataBuffer, 1, dataSize, mapsFile) != dataSize)
    {
      fprintf (stderr, "Failed to export test JSON buffer. %s.\n", strerror (errno));

      fflush (stderr);

      errno = 0;

      assert (false);

      return 1;
    }

    fflush (mapsFile);

    fclose (mapsFile);

    MemoryMapManager manager;

    if (manager.ParseUnixProcessMapsFile (filename) == 0)
    {
      fprintf (stderr, "Failed to process maps file (%s).\n", filename);

      fflush (stderr);

      assert (false);

      return 1;
    }

    kvr::ctx *jsonContext = kvr::ctx::create ();

    JsonNode jsonNode = JsonNodeKvr (*jsonContext->create_value ()->conv_map ());

    if (!manager.PopulateJsonObject (jsonNode))
    {
      fprintf (stderr, "Failed to convert mmap manager to JSON.\n");

      fflush (stderr);

      assert (false);

      return 1;
    }

    if (remove (filename) != 0)
    {
      fprintf (stderr, "Failed to delete test file (%s). %s.\n", filename, strerror (errno));

      fflush (stderr);

      errno = 0;

      assert (false);

      return 1;
    }
  }

  return 0;
}
#endif

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
