/*
	Cute Framework
	Copyright (C) 2020 Randy Gaul https://randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <cute.h>
using namespace cute;

#include <serialize.h>

struct Transform
{
	transform_t world = make_transform();
	transform_t local = make_transform();

	CUTE_INLINE transform_t get() const { return mul(local, world); }
};

CUTE_INLINE error_t Transform_serialize(app_t* app, kv_t* kv, entity_t entity, void* component, void* udata)
{
	Transform* transform = (Transform*)component;
	if (kv_get_state(kv) == KV_STATE_READ) {
		CUTE_PLACEMENT_NEW(transform) Transform;
	}
	return serialize_transform(kv, "world", &transform->world);
}

#endif // TRANSFORM_H