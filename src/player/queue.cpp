/**
 * This file is part of the Zeppelin music player project.
 * Copyright (c) 2013-2014 Zoltan Kovacs, Lajos Santa
 * See http://zeppelin-player.com for more details.
 */

#include <zeppelin/player/queue.h>

using zeppelin::player::QueueItem;
using zeppelin::player::ContainerQueueItem;
using zeppelin::player::File;
using zeppelin::player::Directory;
using zeppelin::player::Album;
using zeppelin::player::Playlist;

// =====================================================================================================================
ContainerQueueItem::ContainerQueueItem()
    : m_index(-1)
{
}

// =====================================================================================================================
void ContainerQueueItem::add(const std::shared_ptr<QueueItem>& item)
{
    m_items.push_back(item);
}

// =====================================================================================================================
void ContainerQueueItem::get(std::vector<int>& i)
{
    if (!isValid())
	return;

    i.push_back(m_index);
    m_items[m_index]->get(i);
}

// =====================================================================================================================
bool ContainerQueueItem::set(std::vector<int>& i)
{
    if (i.empty())
	return false;

    int idx = i.front();
    i.erase(i.begin());

    if (idx < 0 || static_cast<size_t>(idx) >= m_items.size() || !m_items[idx]->set(i))
	return false;

    m_index = idx;

    return true;
}

// =====================================================================================================================
void ContainerQueueItem::remove(std::vector<int>& i)
{
    if (i.empty())
	return;

    int idx = i[0];
    i.erase(i.begin());

    // make sure the index is valid
    if (idx < 0 || static_cast<size_t>(idx) >= m_items.size())
	return;

    // true when the current item on this level changed
    bool currentChanged = false;

    if (i.empty())
    {
	// remove the item
	m_items.erase(m_items.begin() + idx);

	// decrement the index if we removed the item before it
	if (idx < m_index)
	    --m_index;
	else if (idx == m_index)
	    currentChanged = true;
    }
    else
    {
	QueueItem& item = *m_items[idx];

	// remove recursively
	item.remove(i);

	if (item.items().empty())
	{
	    // remove the empty item
	    m_items.erase(m_items.begin() + idx);
	}
	else if (!item.isValid())
	{
	    // the item we removed from got invalidated with the remove, so step to the next one ...
	    ++m_index;
	}
	else
	{
	    // here we return to avoid running the code at the end of the function
	    return;
	}

	currentChanged = true;
    }

    if (currentChanged)
    {
	// at this point the item pointed by m_index is changed so we have to either reset the new item if the index is
	// valid or invalidate the index properly
	if (isValid())
	    m_items[m_index]->reset(FIRST);
	else
	{
	    // Reset the index to -1 if the item is invalid to prevent making it "valid" once a new item is added to
	    // this position. That new item would be valid without calling reset() on it.
	    m_index = -1;
	}
    }
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
const std::shared_ptr<zeppelin::library::File>& ContainerQueueItem::file() const
{
    return m_items[m_index]->file();
}

// =====================================================================================================================
std::vector<std::shared_ptr<QueueItem>> ContainerQueueItem::items() const
{
    return m_items;
}

// =====================================================================================================================
File::File(const std::shared_ptr<zeppelin::library::File>& f)
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
bool File::set(std::vector<int>& i)
{
    // make sure we are at the end of the index
    return i.empty();
}

// =====================================================================================================================
void File::remove(std::vector<int>&)
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
const std::shared_ptr<zeppelin::library::File>& File::file() const
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
Directory::Directory(const std::shared_ptr<zeppelin::library::Directory>& d)
    : m_directory(d)
{
}

// =====================================================================================================================
QueueItem::Type Directory::type() const
{
    return DIRECTORY;
}

// =====================================================================================================================
std::shared_ptr<QueueItem> Directory::clone() const
{
    std::shared_ptr<Directory> d(new Directory());

    d->m_index = m_index;
    d->m_directory = m_directory;

    for (const auto& item : m_items)
	d->m_items.push_back(item->clone());

    return d;
}

// =====================================================================================================================
const zeppelin::library::Directory& Directory::directory() const
{
    return *m_directory;
}

// =====================================================================================================================
Album::Album(const std::shared_ptr<zeppelin::library::Album>& a,
	     const std::vector<std::shared_ptr<zeppelin::library::File>>& files)
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
const zeppelin::library::Album& Album::album() const
{
    return *m_album;
}

// =====================================================================================================================
Playlist::Playlist(int id)
    : m_id(id)
{
}

// =====================================================================================================================
void Playlist::add(const std::shared_ptr<QueueItem>& item)
{
    m_items.push_back(item);
}

// =====================================================================================================================
void Playlist::clear()
{
    m_items.clear();
    m_index = -1;
}

// =====================================================================================================================
int Playlist::getId() const
{
    return m_id;
}

// =====================================================================================================================
QueueItem::Type Playlist::type() const
{
    return PLAYLIST;
}

// =====================================================================================================================
std::shared_ptr<QueueItem> Playlist::clone() const
{
    std::shared_ptr<Playlist> pl = std::make_shared<Playlist>(m_id);

    pl->m_index = m_index;

    for (const auto& item : m_items)
	pl->m_items.push_back(item->clone());

    return pl;
}
