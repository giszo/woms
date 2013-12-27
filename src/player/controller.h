#ifndef PLAYER_CONTORLLER_H_INCLUDED
#define PLAYER_CONTORLLER_H_INCLUDED

#include "decoder.h"
#include "player.h"
#include "fifo.h"
#include "queue.h"

#include <thread/condition.h>
#include <filter/volume.h>

namespace player
{

enum State
{
    STOPPED,
    PLAYING,
    PAUSED
};

struct Status
{
    // the currently played file
    std::shared_ptr<library::File> m_file;
    // the state of the player
    State m_state;
    // position inside the current track in seconds
    unsigned m_position;
    // volume level (0 - 100)
    int m_volume;
};

class Controller
{
    public:
	/// availables commands for the controller
	enum Command
	{
	    PLAY,
	    PAUSE,
	    STOP,
	    PREV,
	    NEXT,
	    GOTO,
	    REMOVE,
	    // sent by the player thread once all samples of the current track have been written to the output
	    SONG_FINISHED,
	    // sent by the decoder thread when the decoding of the current file has been finished
	    DECODER_FINISHED
	};

	Controller(const config::Config& config);

	/// returns the current play queue
	std::shared_ptr<Playlist> getQueue() const;

	/// returns the current status of the player
	Status getStatus();

	/// puts a new file onto the playback queue
	void queue(const std::shared_ptr<library::File>& file);
	/// puts a new album onto the playback queue
	void queue(const std::shared_ptr<library::Album>& album,
		   const std::vector<std::shared_ptr<library::File>>& files);
	/// removes the referenced part of the queue
	void remove(const std::vector<int>& index);

	void play();
	void pause();
	void stop();
	void prev();
	void next();
	void goTo(const std::vector<int>& index);

	/// sets the volume level (level must be between 0 and 100)
	void setVolume(int level);
	/// increases volume level
	void incVolume();
	/// decreases volume level
	void decVolume();

	void command(Command cmd);

	/// the mainloop of the controller
	void run();

    private:
	void setDecoderInput();

    private:
	/// the state of the player
	State m_state;

	Playlist m_decoderQueue;
	bool m_decoderInitialized;

	Playlist m_playerQueue;

	struct CmdBase
	{
	    CmdBase(Command cmd) : m_cmd(cmd) {}
	    Command m_cmd;
	};

	struct GoTo : public CmdBase
	{
	    GoTo(const std::vector<int>& index) : CmdBase(GOTO), m_index(index) {}
	    std::vector<int> m_index;
	};

	struct Remove : public CmdBase
	{
	    Remove(const std::vector<int>& index) : CmdBase(REMOVE), m_index(index) {}
	    std::vector<int> m_index;
	};

	/// controller commands
	std::deque<std::shared_ptr<CmdBase>> m_commands;

	/// fifo for decoder and player threads
	Fifo m_fifo;

	/// input decoder thread filling the sample buffer
	std::unique_ptr<Decoder> m_decoder;

	/// player thread putting decoded samples to the output device
	std::unique_ptr<Player> m_player;

	/// current volume level (between 0 and 100)
	int m_volumeLevel;
	/// volume adjuster filter
	std::shared_ptr<filter::Volume> m_volumeAdj;

	thread::Mutex m_mutex;
	thread::Condition m_cond;
};

}

#endif
