# -*- python -*-

AddOption(
    "--prefix",
    dest = "prefix",
    type = "string",
    nargs = 1,
    action = "store",
    metavar = "DIR",
    help = "installation prefix"
)

env = Environment(PREFIX = GetOption("prefix"))

env["CPPFLAGS"] = ["-O2", "-Wall", "-Werror", "-Wshadow", "-std=c++11", "-pthread"]
env["CPPPATH"] = [Dir("include"), Dir("src")]
env["LINKFLAGS"] = ["-pthread", "-rdynamic"]

env["CXXCOMSTR"] = "Compiling $SOURCE"
env["SHCXXCOMSTR"] = "Compiling $SOURCE"
env["ARCOMSTR"] = "Creating $TARGET"
env["RANLIBCOMSTR"] = "Indexing $TARGET"
env["LINKCOMSTR"] = "Linking $TARGET"
env["SHLINKCOMSTR"] = "Linking $TARGET"

########################################################################################################################
# application library

sources = [
    "logger.cpp",
    "output/baseoutput.cpp",
    "output/alsa.cpp",
    "codec/basecodec.cpp",
    "codec/mp3.cpp",
    "codec/flac.cpp",
    "codec/metadata.cpp",
    "library/musiclibrary.cpp",
    "library/scanner.cpp",
    "library/metaparser.cpp",
    "library/sqlitestorage.cpp",
    "library/file.cpp",
    "library/directory.cpp",
    "library/artist.cpp",
    "library/album.cpp",
    "player/player.cpp",
    "player/decoder.cpp",
    "player/controller.cpp",
    "player/fifo.cpp",
    "player/queue.cpp",
    "player/format.cpp",
    "thread/thread.cpp",
    "thread/condition.cpp",
    "utils/stringutils.cpp",
    "utils/signalhandler.cpp",
    "utils/pidfile.cpp",
    "config/parser.cpp",
    "filter/basefilter.cpp",
    "filter/volume.cpp",
    "filter/resample.cpp",
    "plugin/pluginmanager.cpp"
]

zep_lib = env.StaticLibrary(
    "zeppelin",
    source = ["src/%s" % s for s in sources]
)

########################################################################################################################
# main application

zep = env.Program(
    "zeppelin",
    source = ["src/main.cpp"] + zep_lib,
    LIBS = ["asound", "mpg123", "FLAC", "samplerate", "sqlite3", "jsoncpp", "dl", "boost_locale", "boost_program_options"]
)

# define the defualt target
Default(zep)

########################################################################################################################
# testing

tests = [
    "fifo.cpp",
    "queue.cpp",
    "format.cpp"
]

env.Program(
    "unit_test",
    source = ["tst/%s" % t for t in tests] + ["tst/main.cpp"] + zep_lib,
    LIBS = ["boost_unit_test_framework"]
)

########################################################################################################################
# install

env.Alias("install", env.Install("$PREFIX/usr/bin", zep))
env.Alias("install", env.Install("$PREFIX/usr/include", Dir("include/zeppelin")))

########################################################################################################################
# release

SConscript(dirs = ["release"], exports = ["env"])
