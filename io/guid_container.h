#ifndef _guid_container_h_
#define _guid_container_h_

#include <windows.h>

class guid_container
{
public:
	virtual ~guid_container() {}

	virtual unsigned add( const unsigned long & ) = 0;

	virtual void remove( const unsigned long & ) = 0;

	virtual bool get_guid( unsigned, unsigned long & ) = 0;
};

guid_container * create_guid_container();

#endif