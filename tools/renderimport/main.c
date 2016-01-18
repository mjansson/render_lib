/* main.c  -  Render library importer  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
 *
 * This library provides a cross-platform rendering library in C11 providing
 * basic 2D/3D rendering functionality for projects based on our foundation library.
 *
 * The latest source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/render_lib
 *
 * The foundation library source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/foundation_lib
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#include <foundation/foundation.h>
#include <resource/resource.h>
#include <window/window.h>
#include <render/render.h>

#include "main.h"
#include "errorcodes.h"
#include "shader.h"
#include "program.h"
#include "glsl.h"

typedef struct {
	bool              display_help;
	int               binary;
	string_const_t    source_path;
	string_const_t*   input_files;
} renderimport_input_t;

static renderimport_input_t
renderimport_parse_command_line(const string_const_t* cmdline);

static void
renderimport_print_usage(void);

int
renderimport_import(stream_t* stream, const uuid_t uuid_given) {
	renderimport_type_t type = IMPORTTYPE_UNKNOWN;
	renderimport_type_t guess = IMPORTTYPE_UNKNOWN;
	uuid_t uuid = uuid_given;
	string_const_t path;
	string_const_t extension;
	int ret;
	bool store_import = false;

	path = stream_path(stream);
	extension = path_file_extension(STRING_ARGS(path));
	if (string_equal_nocase(STRING_ARGS(extension), STRING_CONST("shader")))
		guess = IMPORTTYPE_SHADER;
	else if (string_equal_nocase(STRING_ARGS(extension), STRING_CONST("program")))
		guess = IMPORTTYPE_PROGRAM;

	type = renderimport_shader_guess_type(stream);

	if ((type == IMPORTTYPE_UNKNOWN) && (guess != IMPORTTYPE_UNKNOWN))
		type = guess;

	if (type == IMPORTTYPE_UNKNOWN)
		return RENDERIMPORT_RESULT_UNSUPPORTED_INPUT;

	if (uuid_is_null(uuid))
		uuid = resource_import_map_lookup(STRING_ARGS(path));
	
	if (uuid_is_null(uuid) && (type == IMPORTTYPE_SHADER)) {
		uuid = renderimport_shader_check_referenced_uuid(stream);
		store_import = true;
	}

	if (uuid_is_null(uuid)) {
		uuid = uuid_generate_random();
		store_import = true;
	}
	
	if (store_import) {
		if (!resource_import_map_store(STRING_ARGS(path), &uuid)) {
			log_warn(HASH_RESOURCE, WARNING_SUSPICIOUS,
			         STRING_CONST("Unable to open import map file to store new resource"));
			return RENDERIMPORT_RESULT_UNABLE_TO_OPEN_MAP_FILE;
		}
	}

	switch (type) {
	case IMPORTTYPE_PROGRAM:
		ret = renderimport_import_program(stream, uuid);
		break;
	case IMPORTTYPE_SHADER:
		ret = renderimport_import_shader(stream, uuid);
		break;
	case IMPORTTYPE_GLSL_VERTEXSHADER:
		ret = renderimport_import_glsl_vertexshader(stream, uuid);
		break;
	case IMPORTTYPE_GLSL_PIXELSHADER:
		ret = renderimport_import_glsl_pixelshader(stream, uuid);
		break;
	default:
		return RENDERIMPORT_RESULT_UNSUPPORTED_INPUT;
	}

	return ret;
}

int
main_initialize(void) {
	int ret = 0;
	application_t application;
	foundation_config_t foundation_config;
	resource_config_t resource_config;
	window_config_t window_config;
	render_config_t render_config;

	memset(&foundation_config, 0, sizeof(foundation_config));
	memset(&resource_config, 0, sizeof(resource_config));
	memset(&window_config, 0, sizeof(window_config));
	memset(&render_config, 0, sizeof(render_config));

	memset(&application, 0, sizeof(application));
	application.name = string_const(STRING_CONST("renderimport"));
	application.short_name = string_const(STRING_CONST("renderimport"));
	application.config_dir = string_const(STRING_CONST("renderimport"));
	application.flags = APPLICATION_UTILITY;

	log_enable_prefix(false);
	log_set_suppress(0, ERRORLEVEL_WARNING);

	resource_config.enable_local_source = true;
	resource_config.enable_local_cache = true;
	resource_config.enable_remote_source = true;

	if ((ret = foundation_initialize(memory_system_malloc(), application, foundation_config)) < 0)
		return ret;
	if ((ret = resource_module_initialize(resource_config)) < 0)
		return ret;
	if ((ret = window_module_initialize(window_config)) < 0)
		return ret;
	if ((ret = render_module_initialize(render_config)) < 0)
		return ret;

	log_set_suppress(HASH_RESOURCE, ERRORLEVEL_DEBUG);

	return 0;
}

int
main_run(void* main_arg) {
	int result = RENDERIMPORT_RESULT_OK;
	renderimport_input_t input = renderimport_parse_command_line(environment_command_line());

	FOUNDATION_UNUSED(main_arg);

	if (input.display_help) {
		renderimport_print_usage();
		goto exit;
	}

	resource_source_set_path(STRING_ARGS(input.source_path));
	resource_import_register(renderimport_import);

	size_t ifile, fsize;
	for (ifile = 0, fsize = array_size(input.input_files); ifile < fsize; ++ifile) {
		if (resource_import(STRING_ARGS(input.input_files[ifile]), uuid_null()))
			log_infof(HASH_RESOURCE, STRING_CONST("Successfully imported: %.*s"),
			          STRING_FORMAT(input.input_files[ifile]));
		else
			log_warnf(HASH_RESOURCE, WARNING_UNSUPPORTED, STRING_CONST("Failed to import: %.*s"),
			          STRING_FORMAT(input.input_files[ifile]));
	}

exit:

	array_deallocate(input.input_files);

	return result;
}

void
main_finalize(void) {
	render_module_finalize();
	window_module_finalize();
	resource_module_finalize();
	foundation_finalize();
}

renderimport_input_t
renderimport_parse_command_line(const string_const_t* cmdline) {
	renderimport_input_t input;
	size_t arg, asize;

	memset(&input, 0, sizeof(input));

	for (arg = 1, asize = array_size(cmdline); arg < asize; ++arg) {
		if (string_equal(STRING_ARGS(cmdline[arg]), STRING_CONST("--help")))
			input.display_help = true;
		else if (string_equal(STRING_ARGS(cmdline[arg]), STRING_CONST("--source"))) {
			if (arg < asize - 1)
				input.source_path = cmdline[++arg];
		}
		/*else if (string_equal(STRING_ARGS(cmdline[arg]), STRING_CONST("--uuid"))) {
			if (arg < asize - 1) {
				++arg;
				input.uuid = string_to_uuid(STRING_ARGS(cmdline[arg]));
				if (uuid_is_null(input.uuid))
					log_warnf(HASH_RESOURCE, WARNING_INVALID_VALUE, STRING_CONST("Invalid UUID: %.*s"),
					          STRING_FORMAT(cmdline[arg]));
			}
		}
		else if (string_equal(STRING_ARGS(cmdline[arg]), STRING_CONST("--set"))) {
			if (arg < asize - 2) {
				input.key = cmdline[++arg];
				input.value = cmdline[++arg];
			}
		}*/
		else if (string_equal(STRING_ARGS(cmdline[arg]), STRING_CONST("--binary"))) {
			input.binary = 1;
		}
		else if (string_equal(STRING_ARGS(cmdline[arg]), STRING_CONST("--ascii"))) {
			input.binary = 0;
		}
		else if (string_equal(STRING_ARGS(cmdline[arg]), STRING_CONST("--debug"))) {
			log_set_suppress(0, ERRORLEVEL_NONE);
			log_set_suppress(HASH_RESOURCE, ERRORLEVEL_NONE);
			log_set_suppress(HASH_RENDER, ERRORLEVEL_NONE);
		}
		else if (string_equal(STRING_ARGS(cmdline[arg]), STRING_CONST("--")))
			break; //Stop parsing cmdline options
		else {
			array_push(input.input_files, cmdline[arg]);
		}
	}
	error_context_pop();

	bool already_help = input.display_help;
	if (!input.source_path.length)
		input.source_path = resource_source_path();
	if (!already_help && !input.source_path.length) {
		log_errorf(HASH_RESOURCE, ERROR_INVALID_VALUE, STRING_CONST("No source path given"));
		input.display_help = true;
	}
	if (!already_help && !array_size(input.input_files)) {
		log_errorf(HASH_RESOURCE, ERROR_INVALID_VALUE, STRING_CONST("No input files given"));
		input.display_help = true;
	}

	return input;
}

static void
renderimport_print_usage(void) {
	const error_level_t saved_level = log_suppress(0);
	log_set_suppress(0, ERRORLEVEL_DEBUG);
	log_info(0, STRING_CONST(
	             "renderimport usage:\n"
	             "  renderimport [--source <path>] [--ascii] [--binary] [--debug] [--help] <file> <file> ... [--]\n"
	             "    Arguments:\n"
	             "      <file> <file> ...            Any number of input files\n"
	             "    Optional arguments:\n"
	             "      --source <path>              Operate on resource file source structure given by <path>\n"
	             "      --binary                     Write binary files\n"
	             "      --ascii                      Write ASCII files (default)\n"
	             "      --debug                      Enable debug output\n"
	             "      --help                       Display this help message\n"
	             "      --                           Stop processing command line arguments"
	         ));
	log_set_suppress(0, saved_level);
}
