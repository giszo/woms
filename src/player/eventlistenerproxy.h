#ifndef PLAYER_EVENTLISTENERPROXY_H_INCLUDED
#define PLAYER_EVENTLISTENERPROXY_H_INCLUDED

#include <zeppelin/player/eventlistener.h>

#include <set>
#include <memory>

namespace player
{

class EventListenerProxy : public zeppelin::player::EventListener
{
    public:
	void add(const std::shared_ptr<zeppelin::player::EventListener>& listener);

	void started() override;
	void paused() override;
	void stopped() override;

	void positionChanged() override;
	void songChanged() override;

	void queueChanged() override;

	void volumeChanged() override;

    private:
	std::set<std::shared_ptr<zeppelin::player::EventListener>> m_listeners;
};

}

#endif
