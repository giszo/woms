/**
 * This file is part of the Zeppelin music player project.
 * Copyright (c) 2013-2014 Zoltan Kovacs, Lajos Santa
 * See http://zeppelin-player.com for more details.
 */

#include <zeppelin/library/directory.h>

using zeppelin::library::Directory;

// =====================================================================================================================
Directory::Directory(int id, const std::string& name, int parentId)
    : m_id(id),
      m_name(name),
      m_parentId(parentId)
{
}
