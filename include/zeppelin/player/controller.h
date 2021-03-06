/**
 * This file is part of the Zeppelin music player project.
 * Copyright (c) 2013-2014 Zoltan Kovacs, Lajos Santa
 * See http://zeppelin-player.com for more details.
 */

#ifndef ZEPPELIN_CONTROLLER_H_INCLUDED
#define ZEPPELIN_CONTROLLER_H_INCLUDED

#include <memory>
#include <vector>

namespace zeppelin
{
namespace library
{
struct File;
}

namespace player
{

class Playlist;
class EventListener;
class QueueItem;

class Controller
{
    public:
	enum State
	{
	    STOPPED,
	    PLAYING,
	    PAUSED
	};

	struct Status
	{
	    // the currently played file
	    std::shared_ptr<zeppelin::library::File> m_file;
	    // the tree index of the currently played file
	    std::vector<int> m_index;
	    // the state of the player
	    State m_state;
	    // position inside the current track in seconds
	    unsigned m_position;
	    // volume level (0 - 100)
	    int m_volume;
	};

	virtual ~Controller()
	{}

	virtual void addListener(const std::shared_ptr<EventListener>& listener) = 0;

	/// returns the current play queue
	virtual std::shared_ptr<zeppelin::player::Playlist> getQueue() const = 0;

	/// returns the current status of the player
	virtual Status getStatus() = 0;

	/// puts a new item onto the playback queue
	virtual void queue(const std::shared_ptr<zeppelin::player::QueueItem>& item) = 0;
	/// removes the referenced part of the queue
	virtual void remove(const std::vector<int>& index) = 0;
	/// removes all members of the queue
	virtual void removeAll() = 0;

	virtual void play() = 0;
	virtual void pause() = 0;
	virtual void stop() = 0;
	virtual void seek(off_t seconds) = 0;
	virtual void prev() = 0;
	virtual void next() = 0;
	virtual void goTo(const std::vector<int>& index) = 0;

	/// returns the current volume level
	virtual int getVolume() const = 0;
	/// sets the volume level (level must be between 0 and 100)
	virtual void setVolume(int level) = 0;
};

}
}

#endif
