#include "queue.h"

using player::QueueItem;
using player::ContainerQueueItem;
using player::File;
using player::Album;
using player::Playlist;

// =====================================================================================================================
ContainerQueueItem::ContainerQueueItem()
    : m_index(-1)
{
}

// =====================================================================================================================
void ContainerQueueItem::get(std::vector<int>& i)
{
    i.push_back(m_index);

    if (isValid())
	m_items[m_index]->get(i);
}

// =====================================================================================================================
void ContainerQueueItem::set(std::vector<int>& i)
{
    m_index = i.front();
    i.erase(i.begin());

    if (isValid())
	m_items[m_index]->set(i);
}

// =====================================================================================================================
bool ContainerQueueItem::isValid() const
{
    return m_index >= 0 && m_index < static_cast<int>(m_items.size());
}

// =====================================================================================================================
void ContainerQueueItem::reset(Position position)
{
    if (m_items.empty())
	return;

    switch (position)
    {
	case FIRST : m_index = 0; break;
	case LAST : m_index = m_items.size() - 1; break;
    }

    m_items[m_index]->reset(position);
}

// =====================================================================================================================
bool ContainerQueueItem::prev()
{
    if (m_items.empty() || !isValid())
	return false;

    if (m_items[m_index]->prev())
	return true;

    if (m_index == 0)
	return false;

    --m_index;
    m_items[m_index]->reset(LAST);

    return true;
}

// =====================================================================================================================
bool ContainerQueueItem::next()
{
    if (m_items.empty() || !isValid())
	return false;

    if (m_items[m_index]->next())
	return true;

    if (m_index == static_cast<int>(m_items.size()) - 1)
	return false;

    ++m_index;
    m_items[m_index]->reset(FIRST);

    return true;
}

// =====================================================================================================================
const std::shared_ptr<library::File>& ContainerQueueItem::file() const
{
    return m_items[m_index]->file();
}

// =====================================================================================================================
std::vector<std::shared_ptr<QueueItem>> ContainerQueueItem::items() const
{
    return m_items;
}

// =====================================================================================================================
File::File(const std::shared_ptr<library::File>& f)
    : m_file(f)
{
}

// =====================================================================================================================
QueueItem::Type File::type() const
{
    return FILE;
}

// =====================================================================================================================
void File::get(std::vector<int>&)
{
}

// =====================================================================================================================
void File::set(std::vector<int>&)
{
}

// =====================================================================================================================
bool File::isValid() const
{
    return true;
}

// =====================================================================================================================
void File::reset(Position position)
{
}

// =====================================================================================================================
bool File::prev()
{
    return false;
}

// =====================================================================================================================
bool File::next()
{
    return false;
}

// =====================================================================================================================
const std::shared_ptr<library::File>& File::file() const
{
    return m_file;
}

// =====================================================================================================================
std::shared_ptr<QueueItem> File::clone() const
{
    return std::make_shared<File>(m_file);
}

// =====================================================================================================================
std::vector<std::shared_ptr<QueueItem>> File::items() const
{
    return std::vector<std::shared_ptr<QueueItem>>();
}

// =====================================================================================================================
Album::Album(const std::shared_ptr<library::Album>& a,
	     const std::vector<std::shared_ptr<library::File>>& files)
    : m_album(a)
{
    for (const auto& f : files)
	m_items.push_back(std::make_shared<File>(f));
}

// =====================================================================================================================
QueueItem::Type Album::type() const
{
    return ALBUM;
}

// =====================================================================================================================
std::shared_ptr<QueueItem> Album::clone() const
{
    std::shared_ptr<Album> a(new Album());

    a->m_index = m_index;
    a->m_album = m_album;

    for (const auto& item : m_items)
	a->m_items.push_back(item->clone());

    return a;
}

// =====================================================================================================================
const library::Album& Album::album() const
{
    return *m_album;
}

// =====================================================================================================================
void Playlist::add(const std::shared_ptr<library::File>& f)
{
    m_items.push_back(std::make_shared<File>(f));
}

// =====================================================================================================================
void Playlist::add(const std::shared_ptr<library::Album>& album,
		   const std::vector<std::shared_ptr<library::File>>& files)
{
    m_items.push_back(std::make_shared<Album>(album, files));
}

// =====================================================================================================================
QueueItem::Type Playlist::type() const
{
    return PLAYLIST;
}

// =====================================================================================================================
std::shared_ptr<QueueItem> Playlist::clone() const
{
    std::shared_ptr<Playlist> pl = std::make_shared<Playlist>();

    pl->m_index = m_index;

    for (const auto& item : m_items)
	pl->m_items.push_back(item->clone());

    return pl;
}