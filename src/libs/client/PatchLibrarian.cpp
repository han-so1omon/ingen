/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#include "PatchLibrarian.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <algorithm>
#include "PatchModel.h"
#include "NodeModel.h"
#include "ConnectionModel.h"
#include "PortModel.h"
#include "PresetModel.h"
#include "ModelEngineInterface.h"
#include "PluginModel.h"
#include "util/Path.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <utility> // for pair, make_pair
#include <cassert>
#include <cstring>
#include <string>
#include <cstdlib>  // for atof
#include <cmath>

using std::string; using std::vector; using std::pair;
using std::cerr; using std::cout; using std::endl;

namespace Ingen {
namespace Client {

	
/** Searches for the filename passed in the path, returning the full
 * path of the file, or the empty string if not found.
 *
 * This function tries to be as friendly a black box as possible - if the path
 * passed is an absolute path and the file is found there, it will return
 * that path, etc.
 *
 * additional_path is a list (colon delimeted as usual) of additional
 * directories to look in.  ie the directory the parent patch resides in would
 * be a good idea to pass as additional_path, in the case of a subpatch.
 */
string
PatchLibrarian::find_file(const string& filename, const string& additional_path)
{
	string search_path = additional_path + ":" + _patch_search_path;
	
	// Try to open the raw filename first
	std::ifstream is(filename.c_str(), std::ios::in);
	if (is.good()) {
		is.close();
		return filename;
	}
	
	string directory;
	string full_patch_path = "";
	
	while (search_path != "") {
		directory = search_path.substr(0, search_path.find(':'));
		if (search_path.find(':') != string::npos)
			search_path = search_path.substr(search_path.find(':')+1);
		else
			search_path = "";

		full_patch_path = directory +"/"+ filename;
		
		std::ifstream is;
		is.open(full_patch_path.c_str(), std::ios::in);
	
		if (is.good()) {
			is.close();
			return full_patch_path;
		} else {
			cerr << "[PatchLibrarian] Could not find patch file " << full_patch_path << endl;
		}
	}

	return "";
}


string
PatchLibrarian::translate_load_path(const string& path)
{
	std::map<string,string>::iterator t = _load_path_translations.find(path);
	
	if (t != _load_path_translations.end()) {
		return (*t).second;
	} else {
		assert(Path::is_valid(path));
		return path;
	}
}


/** Save a patch from a PatchModel to a filename.
 *
 * The filename passed is the true filename the patch will be saved to (with no prefixing or anything
 * like that), and the patch_model's filename member will be set accordingly.
 *
 * This will break if:
 * - The filename does not have an extension (ie contain a ".")
 * - The patch_model has no (Ingen) path
 */
#if 0
void
PatchLibrarian::save_patch(CountedPtr<PatchModel> patch_model, const string& filename, bool recursive)
{
	assert(filename != "");
	assert(patch_model->path() != "");
	
	cout << "Saving patch " << patch_model->path() << " to " << filename << endl;

	if (patch_model->filename() != filename)
		cerr << "Warning: Saving patch to file other than filename stored in model." << endl;

	string dir = filename.substr(0, filename.find_last_of("/"));
	
	NodeModel* nm = NULL;
	
	xmlDocPtr  xml_doc = NULL;
    xmlNodePtr xml_root_node = NULL;
	xmlNodePtr xml_node = NULL;
	xmlNodePtr xml_child_node = NULL;
	//xmlNodePtr xml_grandchild_node = NULL;
	
    xml_doc = xmlNewDoc((xmlChar*)"1.0");
    xml_root_node = xmlNewNode(NULL, (xmlChar*)"patch");
    xmlDocSetRootElement(xml_doc, xml_root_node);

	const size_t temp_buf_length = 255;
	char temp_buf[temp_buf_length];
	
	string patch_name;
	if (patch_model->path() != "/") {
	  patch_name = patch_model->path().name();
	} else {
	  patch_name = filename;
	  if (patch_name.find("/") != string::npos)
	    patch_name = patch_name.substr(patch_name.find_last_of("/") + 1);
	  if (patch_name.find(".") != string::npos)
	    patch_name = patch_name.substr(0, patch_name.find_last_of("."));
	}

	assert(patch_name.length() > 0);
	xml_node = xmlNewChild(xml_root_node, NULL, (xmlChar*)"name",
			       (xmlChar*)patch_name.c_str());
	
	snprintf(temp_buf, temp_buf_length, "%zd", patch_model->poly());
	xml_node = xmlNewChild(xml_root_node, NULL, (xmlChar*)"polyphony", (xmlChar*)temp_buf);
	
	// Write metadata
	for (MetadataMap::const_iterator i = patch_model->metadata().begin();
			i != patch_model->metadata().end(); ++i) {
		cerr << "FIXME: metadata save" << endl;
		// Dirty hack, don't save coordinates in patch file
		//if (i->first != "module-x" && i->first != "module-y"
		//		&& i->first != "filename")
		//	xml_node = xmlNewChild(xml_root_node, NULL,
		//		(xmlChar*)(*i).first.c_str(), (xmlChar*)(*i).second.c_str());

		assert((*i).first != "node");
		assert((*i).first != "subpatch");
		assert((*i).first != "name");
		assert((*i).first != "polyphony");
		assert((*i).first != "preset");
	}
	
	// Save nodes and subpatches
	for (NodeModelMap::const_iterator i = patch_model->nodes().begin(); i != patch_model->nodes().end(); ++i) {
		nm = i->second.get();
		
		if (nm->plugin()->type() == PluginModel::Patch) {  // Subpatch
			CountedPtr<PatchModel> spm = PtrCast<PatchModel>(i->second);
			assert(spm);

			xml_node = xmlNewChild(xml_root_node, NULL, (xmlChar*)"subpatch", NULL);
			
			xml_child_node = xmlNewChild(xml_node, NULL, (xmlChar*)"name", (xmlChar*)spm->path().name().c_str());
			
			string ref_filename;
			// No path
			if (spm->filename() == "") {
				ref_filename = spm->path().name() + ".om";
				cerr << "FIXME: subpatch filename" << endl;
				//spm->filename(dir +"/"+ ref_filename);
			// Absolute path
			} else if (spm->filename().substr(0, 1) == "/") {
				// Attempt to make it a relative path, if it's undernath this patch's dir
				if (dir.substr(0, 1) == "/" && spm->filename().substr(0, dir.length()) == dir) {
					ref_filename = spm->filename().substr(dir.length()+1);
				} else { // FIXME: not good
					ref_filename = spm->filename().substr(spm->filename().find_last_of("/")+1);
					cerr << "FIXME: subpatch filename (2)" << endl;
					//spm->filename(dir +"/"+ ref_filename);
				}
			} else {
				ref_filename = spm->filename();
			}
			
			xml_child_node = xmlNewChild(xml_node, NULL, (xmlChar*)"filename", (xmlChar*)ref_filename.c_str());
			
			snprintf(temp_buf, temp_buf_length, "%zd", spm->poly());
			xml_child_node = xmlNewChild(xml_node, NULL,  (xmlChar*)"polyphony", (xmlChar*)temp_buf);
			
			// Write metadata
			for (MetadataMap::const_iterator i = nm->metadata().begin();
					i != nm->metadata().end(); ++i) {	
				cerr << "FIXME: save metadata\n";
				// Dirty hack, don't save metadata that would be in patch file
				/*if ((*i).first != "polyphony" && (*i).first != "filename"
					&& (*i).first != "author" && (*i).first != "description")
					xml_child_node = xmlNewChild(xml_node, NULL,
						(xmlChar*)(*i).first.c_str(), (xmlChar*)(*i).second.c_str());*/
			}
	
			if (recursive)
				save_patch(spm, spm->filename(), true);

		} else {  // Normal node
			xml_node = xmlNewChild(xml_root_node, NULL, (xmlChar*)"node", NULL);
			
			xml_child_node = xmlNewChild(xml_node, NULL, (xmlChar*)"name", (xmlChar*)nm->path().name().c_str());
			
			if (!nm->plugin()) break;
	
			xml_child_node = xmlNewChild(xml_node, NULL,  (xmlChar*)"polyphonic",
				(xmlChar*)((nm->polyphonic()) ? "true" : "false"));
				
			xml_child_node = xmlNewChild(xml_node, NULL,  (xmlChar*)"type",
				(xmlChar*)nm->plugin()->type_string());
			/*
			xml_child_node = xmlNewChild(xml_node, NULL,  (xmlChar*)"plugin-label",
				(xmlChar*)(nm->plugin()->plug_label().c_str()));
	
			if (nm->plugin()->type() != PluginModel::Internal) {
				xml_child_node = xmlNewChild(xml_node, NULL,  (xmlChar*)"library-name",
					(xmlChar*)(nm->plugin()->lib_name().c_str()));
			}*/
			xml_child_node = xmlNewChild(xml_node, NULL,  (xmlChar*)"plugin-uri",
				(xmlChar*)(nm->plugin()->uri().c_str()));
		
			// Write metadata
			for (MetadataMap::const_iterator i = nm->metadata().begin(); i != nm->metadata().end(); ++i) {
				cerr << "FIXME: Save metadata\n";
				/*
				// DSSI _hack_ (FIXME: fix OSC to be more like this and not smash DSSI into metadata?)
				if ((*i).first.substr(0, 16) == "dssi-configure--") {
					xml_child_node = xmlNewChild(xml_node, NULL, (xmlChar*)"dssi-configure", NULL);
					xml_grandchild_node = xmlNewChild(xml_child_node, NULL,
						(xmlChar*)"key", (xmlChar*)(*i).first.substr(16).c_str());
					xml_grandchild_node = xmlNewChild(xml_child_node, NULL,
							(xmlChar*)"value", (xmlChar*)(*i).second.c_str());
				} else if ((*i).first == "dssi-program") {
					xml_child_node = xmlNewChild(xml_node, NULL, (xmlChar*)"dssi-program", NULL);
					xml_grandchild_node = xmlNewChild(xml_child_node, NULL,
						(xmlChar*)"bank", (xmlChar*)(*i).second.substr(0, (*i).second.find("/")).c_str());
					xml_grandchild_node = xmlNewChild(xml_child_node, NULL,
						(xmlChar*)"program", (xmlChar*)(*i).second.substr((*i).second.find("/")+1).c_str());
				} else {
					xml_child_node = xmlNewChild(xml_node, NULL,
						(xmlChar*)(*i).first.c_str(), (xmlChar*)(*i).second.c_str());
				}
				*/
			}
	
			// Write port metadata, if necessary
			for (PortModelList::const_iterator i = nm->ports().begin(); i != nm->ports().end(); ++i) {
				cerr << "FIXME: save metadata\n";
				/*
				const PortModel* const pm = i->get();
				if (pm->is_input() && pm->user_min() != pm->min_val() || pm->user_max() != pm->max_val()) {
					xml_child_node = xmlNewChild(xml_node, NULL, (xmlChar*)"port", NULL);
					xml_grandchild_node = xmlNewChild(xml_child_node, NULL, (xmlChar*)"name",
						(xmlChar*)pm->path().name().c_str());
					snprintf(temp_buf, temp_buf_length, "%f", pm->user_min());
					xml_grandchild_node = xmlNewChild(xml_child_node, NULL, (xmlChar*)"user-min", (xmlChar*)temp_buf);
					snprintf(temp_buf, temp_buf_length, "%f", pm->user_max());
					xml_grandchild_node = xmlNewChild(xml_child_node, NULL, (xmlChar*)"user-max", (xmlChar*)temp_buf);
				}*/	
			}
		}
	}

	// Save connections
	
	const list<CountedPtr<ConnectionModel> >& cl = patch_model->connections();
	const ConnectionModel* c = NULL;
	
	for (list<CountedPtr<ConnectionModel> >::const_iterator i = cl.begin(); i != cl.end(); ++i) {
		c = (*i).get();
		xml_node = xmlNewChild(xml_root_node, NULL, (xmlChar*)"connection", NULL);
		xml_child_node = xmlNewChild(xml_node, NULL, (xmlChar*)"source-node",
			(xmlChar*)c->src_port_path().parent().name().c_str());
		xml_child_node = xmlNewChild(xml_node, NULL, (xmlChar*)"source-port",
			(xmlChar*)c->src_port_path().name().c_str());
		xml_child_node = xmlNewChild(xml_node, NULL, (xmlChar*)"destination-node",
			(xmlChar*)c->dst_port_path().parent().name().c_str());
		xml_child_node = xmlNewChild(xml_node, NULL, (xmlChar*)"destination-port",
			(xmlChar*)c->dst_port_path().name().c_str());
	}
	
    // Save control values (ie presets eventually, right now just current control vals)
	
	xmlNodePtr xml_preset_node = xmlNewChild(xml_root_node, NULL, (xmlChar*)"preset", NULL);
	xml_node = xmlNewChild(xml_preset_node, NULL, (xmlChar*)"name", (xmlChar*)"default");

	PortModel* pm = NULL;

	// Save node port controls
	for (NodeModelMap::const_iterator n = patch_model->nodes().begin(); n != patch_model->nodes().end(); ++n) {
		nm = n->second.get();
		for (PortModelList::const_iterator p = nm->ports().begin(); p != nm->ports().end(); ++p) {
			pm = (*p).get();
			if (pm->is_input() && pm->is_control()) {
				float val = pm->value();
				xml_node = xmlNewChild(xml_preset_node, NULL, (xmlChar*)"control",  NULL);
				xml_child_node = xmlNewChild(xml_node, NULL, (xmlChar*)"node-name",
					(xmlChar*)nm->path().name().c_str());
				xml_child_node = xmlNewChild(xml_node, NULL, (xmlChar*)"port-name",
					(xmlChar*)pm->path().name().c_str());
				snprintf(temp_buf, temp_buf_length, "%f", val);
				xml_child_node = xmlNewChild(xml_node, NULL, (xmlChar*)"value",
					(xmlChar*)temp_buf);
			}
		}
	}
	
	// Save patch port controls
	for (PortModelList::const_iterator p = patch_model->ports().begin();
			p != patch_model->ports().end(); ++p) {
		pm = (*p).get();
		if (pm->is_input() && pm->is_control()) {
			float val = pm->value();
			xml_node = xmlNewChild(xml_preset_node, NULL, (xmlChar*)"control",  NULL);
			xml_child_node = xmlNewChild(xml_node, NULL, (xmlChar*)"port-name",
				(xmlChar*)pm->path().name().c_str());
			snprintf(temp_buf, temp_buf_length, "%f", val);
			xml_child_node = xmlNewChild(xml_node, NULL, (xmlChar*)"value",
				(xmlChar*)temp_buf);
		}
	}
	
	xmlSaveFormatFile(filename.c_str(), xml_doc, 1); // 1 == pretty print

    xmlFreeDoc(xml_doc);
    xmlCleanupParser();
}
#endif


/** Load a patch in to the engine (and client) from a patch file.
 *
 * The name and poly from the passed PatchModel are used.  If the name is
 * the empty string, the name will be loaded from the file.  If the poly
 * is 0, it will be loaded from file.  Otherwise the given values will
 * be used.
 *
 * @param wait If true the patch will be checked for existence before
 * loading everything in to it (to prevent messing up existing patches
 * that exist at the path this one should load as).
 *
 * @param existing If true, the patch will be loaded into a currently
 * existing patch (ie a merging will take place).  Errors will result
 * if Nodes of conflicting names exist.
 *
 * @param parent_path Patch to load this patch as a child of (empty string to load
 * to the root patch)
 *
 * @param name Name of this patch (loaded/generated if the empty string)
 *
 * @param initial_data will be set last, so values passed there will override
 * any values loaded from the patch file.
 *
 * Returns the path of the newly created patch.
 */
string
PatchLibrarian::load_patch(const string& filename,
	                       const string& parent_path,
	                       const string& name,
	                       size_t        poly,
	                       MetadataMap   initial_data,
	                       bool          existing)
{
	cerr << "[PatchLibrarian] Loading patch " << filename << "" << endl;

	Path path = "/"; // path of the new patch

	const bool load_name = (name == "");
	const bool load_poly = (poly == 0);
	
	if (initial_data.find("filename") == initial_data.end())
		initial_data["filename"] = Atom(filename.c_str()); // FIXME: URL?

	xmlDocPtr doc = xmlParseFile(filename.c_str());

	if (!doc) {
		cerr << "Unable to parse patch file." << endl;
		return "";
	}

	xmlNodePtr cur = xmlDocGetRootElement(doc);

	if (!cur) {
		cerr << "Empty document." << endl;
		xmlFreeDoc(doc);
		return "";
	}

	if (xmlStrcmp(cur->name, (const xmlChar*) "patch")) {
		cerr << "File is not an Ingen patch file (root node != <patch>)" << endl;
		xmlFreeDoc(doc);
		return "";
	}

	xmlChar* key = NULL;
	cur = cur->xmlChildrenNode;

	// Load Patch attributes
	while (cur != NULL) {
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"name"))) {
			if (load_name) {
				assert(key != NULL);
				if (parent_path != "")
					path = Path(parent_path).base() + Path::nameify((char*)key);
			}
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"polyphony"))) {
			if (load_poly) {
				poly = atoi((char*)key);
			}
		} else if (xmlStrcmp(cur->name, (const xmlChar*)"connection")
				&& xmlStrcmp(cur->name, (const xmlChar*)"node")
				&& xmlStrcmp(cur->name, (const xmlChar*)"subpatch")
				&& xmlStrcmp(cur->name, (const xmlChar*)"filename")
				&& xmlStrcmp(cur->name, (const xmlChar*)"preset")) {
			// Don't know what this tag is, add it as metadata without overwriting
			// (so caller can set arbitrary parameters which will be preserved)
			if (key)
				if (initial_data.find((const char*)cur->name) == initial_data.end())
					initial_data[(const char*)cur->name] = (const char*)key;
		}
		
		xmlFree(key);
		key = NULL; // Avoid a (possible?) double free

		cur = cur->next;
	}
	
	if (poly == 0)
		poly = 1;

	// Create it, if we're not merging
	if (!existing)
		_engine->create_patch_with_data(path, poly, initial_data);

	// Load nodes
	cur = xmlDocGetRootElement(doc)->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"node")))
			load_node(path, doc, cur);
			
		cur = cur->next;
	}

	// Load subpatches
	cur = xmlDocGetRootElement(doc)->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"subpatch"))) {
			load_subpatch(path, doc, cur);
		}
		cur = cur->next;
	}
	
	// Load connections
	cur = xmlDocGetRootElement(doc)->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"connection"))) {
			load_connection(path, doc, cur);
		}
		cur = cur->next;
	}
	
	
	// Load presets (control values)
	cerr << "FIXME: load preset\n";
	/*cur = xmlDocGetRootElement(doc)->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"preset"))) {
			load_preset(pm, doc, cur);
			assert(preset_model != NULL);
			if (preset_model->name() == "default")
				_engine->set_preset(pm->path(), preset_model);
		}
		cur = cur->next;
	}
	*/
	
	xmlFreeDoc(doc);
	xmlCleanupParser();

	// Done above.. late enough?
	//_engine->set_metadata_map(path, initial_data);

	if (!existing)
		_engine->enable_patch(path);

	_load_path_translations.clear();

	return path;
}


/** Build a NodeModel given a pointer to a Node in a patch file.
 */
bool
PatchLibrarian::load_node(const Path& parent, xmlDocPtr doc, const xmlNodePtr node)
{
	xmlChar* key;
	xmlNodePtr cur = node->xmlChildrenNode;
	
	string path = "";
	bool   polyphonic = false;

	string plugin_uri;
	
	string plugin_type;  // deprecated
	string library_name; // deprecated
	string plugin_label; // deprecated

	MetadataMap initial_data;

	while (cur != NULL) {
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"name"))) {
			path = parent.base() + Path::nameify((char*)key);
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"polyphonic"))) {
			polyphonic = !strcmp((char*)key, "true");
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"type"))) {
			plugin_type = (const char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"library-name"))) {
			library_name = (char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"plugin-label"))) {
			plugin_label = (char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"plugin-uri"))) {
			plugin_uri = (char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"port"))) {
			cerr << "FIXME: load port\n";
#if 0
			xmlNodePtr child = cur->xmlChildrenNode;
			
			string port_name;
			float user_min = 0.0;
			float user_max = 0.0;
			
			while (child != NULL) {
				key = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
				
				if ((!xmlStrcmp(child->name, (const xmlChar*)"name"))) {
					port_name = Path::nameify((char*)key);
				} else if ((!xmlStrcmp(child->name, (const xmlChar*)"user-min"))) {
					user_min = atof((char*)key);
				} else if ((!xmlStrcmp(child->name, (const xmlChar*)"user-max"))) {
					user_max = atof((char*)key);
				}
				
				xmlFree(key);
				key = NULL; // Avoid a (possible?) double free
		
				child = child->next;
			}

			assert(path.length() > 0);
			assert(Path::is_valid(path));

			// FIXME: /nasty/ assumptions
			CountedPtr<PortModel> pm(new PortModel(Path(path).base() + port_name,
					PortModel::CONTROL, PortModel::INPUT, PortModel::NONE,
					0.0, user_min, user_max));
			//pm->set_parent(nm);
			nm->add_port(pm);
#endif

		// DSSI hacks.  Stored in the patch files as special elements, but sent to
		// the engine as normal metadata with specially formatted key/values.  Not
		// sure if this is the best way to go about this, but it's the least damaging
		// right now
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"dssi-program"))) {
			cerr << "FIXME: load dssi program\n";
#if 0
			xmlNodePtr child = cur->xmlChildrenNode;
			
			string bank;
			string program;
			
			while (child != NULL) {
				key = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
				
				if ((!xmlStrcmp(child->name, (const xmlChar*)"bank"))) {
					bank = (char*)key;
				} else if ((!xmlStrcmp(child->name, (const xmlChar*)"program"))) {
					program = (char*)key;
				}
				
				xmlFree(key);
				key = NULL; // Avoid a (possible?) double free
				child = child->next;
			}
			nm->set_metadata("dssi-program", Atom(bank.append("/").append(program).c_str()));
#endif
			
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"dssi-configure"))) {
			cerr << "FIXME: load dssi configure\n";
#if 0
			xmlNodePtr child = cur->xmlChildrenNode;
			
			string dssi_key;
			string dssi_value;
			
			while (child != NULL) {
				key = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
				
				if ((!xmlStrcmp(child->name, (const xmlChar*)"key"))) {
					dssi_key = (char*)key;
				} else if ((!xmlStrcmp(child->name, (const xmlChar*)"value"))) {
					dssi_value = (char*)key;
				}
				
				xmlFree(key);
				key = NULL; // Avoid a (possible?) double free
		
				child = child->next;
			}
			nm->set_metadata(string("dssi-configure--").append(dssi_key), Atom(dssi_value.c_str()));
#endif		
		} else {  // Don't know what this tag is, add it as metadata
			if (key) {

				// Hack to make module-x and module-y set as floats
				char* endptr = NULL;
				float fval = strtof((const char*)key, &endptr);
				if (endptr != (char*)key && *endptr == '\0')
					initial_data[(const char*)cur->name] = Atom(fval);
				else
					initial_data[(const char*)cur->name] = Atom((const char*)key);
			}
		}
		xmlFree(key);
		key = NULL;

		cur = cur->next;
	}
	
	if (path == "") {
		cerr << "[PatchLibrarian] Malformed patch file (node tag has empty children)" << endl;
		cerr << "[PatchLibrarian] Node ignored." << endl;
		return false;
	}

	// Compatibility hacks for old patches that represent patch ports as nodes
	if (plugin_uri == "") {
		cerr << "WARNING: Loading deprecated Node.  Resave! " << path << endl;
		bool is_port = false;

		if (plugin_type == "Internal") {
			if (plugin_label == "audio_input") {
				_engine->create_port(path, "AUDIO", false);
				is_port = true;
			} else if (plugin_label == "audio_output") {
				_engine->create_port(path, "AUDIO", true);
				is_port = true;
			} else if (plugin_label == "control_input") {
				_engine->create_port(path, "CONTROL", false);
				is_port = true;
			} else if (plugin_label == "control_output" ) {
				_engine->create_port(path, "CONTROL", true);
				is_port = true;
			} else if (plugin_label == "midi_input") {
				_engine->create_port(path, "MIDI", false);
				is_port = true;
			} else if (plugin_label == "midi_output" ) {
				_engine->create_port(path, "MIDI", true);
				is_port = true;
			}
		}

		if (is_port) {
			const string old_path = path;
			const string new_path = Path::pathify(old_path);

			// Set up translations (for connections etc) to alias both the old
			// module path and the old module/port path to the new port path
			_load_path_translations[old_path] = new_path;
			_load_path_translations[old_path + "/in"] = new_path;
			_load_path_translations[old_path + "/out"] = new_path;

			path = new_path;

			_engine->set_metadata_map(path, initial_data);

			return CountedPtr<NodeModel>();

		} else {
			if (plugin_label  == "note_in") {
				plugin_uri = "ingen:note_node";
			} else if (plugin_label == "control_input") {
				plugin_uri = "ingen:control_node";
			} else if (plugin_label == "transport") {
				plugin_uri = "ingen:transport_node";
			} else if (plugin_label == "trigger_in") {
				plugin_uri = "ingen:trigger_node";
			} else {
				cerr << "WARNING: Unknown deprecated node (label " << plugin_label
					<< ")." << endl;
			}

			if (plugin_uri != "")
				_engine->create_node(path, plugin_uri, polyphonic);
			else
				_engine->create_node(path, plugin_type, library_name, plugin_label, polyphonic);
		
			_engine->set_metadata_map(path, initial_data);

			return true;
		}

	// Not deprecated
	} else {
		_engine->create_node(path, plugin_uri, polyphonic);
		_engine->set_metadata_map(path, initial_data);
		return true;
	}

	// (shouldn't get here)
}


bool
PatchLibrarian::load_subpatch(const Path& parent, xmlDocPtr doc, const xmlNodePtr subpatch)
{
	xmlChar *key;
	xmlNodePtr cur = subpatch->xmlChildrenNode;
	
	string name     = "";
	string filename = "";
	size_t poly     = 0;
	
	MetadataMap initial_data;

	while (cur != NULL) {
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"name"))) {
			name = (const char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"polyphony"))) {
			poly = atoi((const char*)key);
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"filename"))) {
			filename = (const char*)key;
		} else {  // Don't know what this tag is, add it as metadata
			if (key != NULL && strlen((const char*)key) > 0)
				initial_data[(const char*)cur->name] = Atom((const char*)key);
		}
		xmlFree(key);
		key = NULL;

		cur = cur->next;
	}

	// load_patch sets the passed metadata last, so values stored in the parent
	// will override values stored in the child patch file
	string path = load_patch(filename, parent, name, poly, initial_data, false);
	
	return false;
}


/** Build a ConnectionModel given a pointer to a connection in a patch file.
 */
bool
PatchLibrarian::load_connection(const Path& parent, xmlDocPtr doc, const xmlNodePtr node)
{
	xmlChar *key;
	xmlNodePtr cur = node->xmlChildrenNode;
	
	string source_node, source_port, dest_node, dest_port;
	
	while (cur != NULL) {
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"source-node"))) {
			source_node = (char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"source-port"))) {
			source_port = (char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"destination-node"))) {
			dest_node = (char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"destination-port"))) {
			dest_port = (char*)key;
		}
		
		xmlFree(key);
		key = NULL; // Avoid a (possible?) double free

		cur = cur->next;
	}

	if (source_node == "" || source_port == "" || dest_node == "" || dest_port == "") {
		cerr << "ERROR: Malformed patch file (connection tag has empty children)" << endl;
		cerr << "ERROR: Connection ignored." << endl;
		return false;
	}

	// Compatibility fixes for old (fundamentally broken) patches
	source_node = Path::nameify(source_node);
	source_port = Path::nameify(source_port);
	dest_node = Path::nameify(dest_node);
	dest_port = Path::nameify(dest_port);

	_engine->connect(
		translate_load_path(parent.base() + source_node +"/"+ source_port),
		translate_load_path(parent.base() + dest_node +"/"+ dest_port));
	
	return true;
}


/** Build a PresetModel given a pointer to a preset in a patch file.
 */
bool
PatchLibrarian::load_preset(const Path& parent, xmlDocPtr doc, const xmlNodePtr node)
{
	cerr << "FIXME: load preset\n";
#if 0
	xmlNodePtr cur = node->xmlChildrenNode;
	xmlChar* key;

	PresetModel* pm = new PresetModel(patch->path().base());
	
	while (cur != NULL) {
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);

		if ((!xmlStrcmp(cur->name, (const xmlChar*)"name"))) {
			assert(key != NULL);
			pm->name((char*)key);
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"control"))) {
			xmlNodePtr child = cur->xmlChildrenNode;
	
			string node_name = "", port_name = "";
			float val = 0.0;
			
			while (child != NULL) {
				key = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
				
				if ((!xmlStrcmp(child->name, (const xmlChar*)"node-name"))) {
					node_name = (char*)key;
				} else if ((!xmlStrcmp(child->name, (const xmlChar*)"port-name"))) {
					port_name = (char*)key;
				} else if ((!xmlStrcmp(child->name, (const xmlChar*)"value"))) {
					val = atof((char*)key);
				}
				
				xmlFree(key);
				key = NULL; // Avoid a (possible?) double free
		
				child = child->next;
			}

			// Compatibility fixes for old patch files
			node_name = Path::nameify(node_name);
			port_name = Path::nameify(port_name);
			
			if (port_name == "") {
				string msg = "Unable to parse control in patch file ( node = ";
				msg.append(node_name).append(", port = ").append(port_name).append(")");
				cerr << "ERROR: " << msg << endl;
				//m_client_hooks->error(msg);
			} else {
				// FIXME: temporary compatibility, remove any slashes from port name
				// remove this soon once patches have migrated
				string::size_type slash_index;
				while ((slash_index = port_name.find("/")) != string::npos)
					port_name[slash_index] = '-';
				pm->add_control(node_name, port_name, val);
			}
		}
		xmlFree(key);
		key = NULL;
		cur = cur->next;
	}
	if (pm->name() == "") {
		cerr << "Preset in patch file has no name." << endl;
		//m_client_hooks->error("Preset in patch file has no name.");
		pm->name("Unnamed");
	}

	return pm;
#endif
	return false;
}

} // namespace Client
} // namespace Ingen
