/**
 * This file is part of the Zeppelin music player project.
 * Copyright (c) 2013-2014 Zoltan Kovacs, Lajos Santa
 * See http://zeppelin-player.com for more details.
 */

#ifndef ZEPPELIN_LIBRARY_MUSICLIBRARY_H_INCLUDED
#define ZEPPELIN_LIBRARY_MUSICLIBRARY_H_INCLUDED

namespace zeppelin
{
namespace library
{

class Storage;

class MusicLibrary
{
    public:
	struct Status
	{
	    bool m_scannerRunning;
	    bool m_metaParserRunning;
	};

	virtual ~MusicLibrary()
	{}

	// returns the current status of the library
	virtual Status getStatus() = 0;

	// returns the storage engine of the library
	virtual Storage& getStorage() = 0;

	// initiates a new scanning on the library
	virtual void scan() = 0;
};

}
}

#endif
