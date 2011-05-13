/* This file is part of Ingen.
 * Copyright 2010-2011 David Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef INGEN_ENGINE_LV2_REQUEST_RUN_FEATURE_HPP
#define INGEN_ENGINE_LV2_REQUEST_RUN_FEATURE_HPP

#include "lv2/lv2plug.in/ns/ext/contexts/contexts.h"

#include "raul/log.hpp"

#include "shared/LV2Features.hpp"

#include "Driver.hpp"
#include "Engine.hpp"
#include "MessageContext.hpp"
#include "NodeImpl.hpp"
#include "PortImpl.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {

struct RequestRunFeature : public Ingen::Shared::LV2Features::Feature {
	struct Data {
		inline Data(Shared::World* w, Node* n) : world(w), node(n) {}
		Shared::World* world;
		Node*          node;
	};

	static void request_run(LV2_Contexts_Request_Run_Data data_ptr,
	                        uint32_t                      context_uri) {
		Data* data = reinterpret_cast<Data*>(data_ptr);
		if (!data->world->local_engine())
			return;

		Engine* engine = (Engine*)data->world->local_engine().get();
		engine->message_context()->run(
			dynamic_cast<NodeImpl*>(data->node),
			engine->driver()->frame_time());
	}

	static void delete_feature(LV2_Feature* feature) {
		delete (Data*)feature->data;
		free(feature);
	}

	SharedPtr<LV2_Feature> feature(Shared::World* world, Node* n) {
		const NodeImpl* node = dynamic_cast<const NodeImpl*>(n);
		if (!node)
			return SharedPtr<LV2_Feature>();

		typedef LV2_Contexts_Request_Run_Feature Feature;
		Feature* data = (Feature*)malloc(sizeof(Feature));
		data->data        = new Data(world, n);
		data->request_run = &request_run;

		LV2_Feature* f = (LV2_Feature*)malloc(sizeof(LV2_Feature));
		f->URI  = LV2_CONTEXTS_URI "#RequestRunFeature";
		f->data = data;

		return SharedPtr<LV2_Feature>(f, &delete_feature);
	}
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_LV2_REQUEST_RUN_FEATURE_HPP
