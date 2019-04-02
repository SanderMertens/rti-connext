#include <bake>
#include <include/connext.h>

#define RTI_CONNEXT_PACKAGE "rti.connext"
#define RTI_DDSGEN_PATH "bin/rtiddsgen"
#define RTI_HOME "NDDSHOME"
#define RTI_PLATFORM "NDDSPLATFORM"

#define RTI_IDL_HEADER ".h"
#define RTI_IDL_PLUGIN_HEADER "Plugin.h"
#define RTI_IDL_SUPPORT_HEADER "Support.h"

#define DRIVER_IDL_ATTR "idl"
#define DRIVER_PUBLIC_HEADERS_ATTR "public-headers"

static
void init(
    bake_driver_api *driver,
    bake_config *config,
    bake_project *project)
{
    driver->use(RTI_CONNEXT_PACKAGE);
}

static
void generate(
    bake_driver_api *driver,
    bake_config *config,
    bake_project *project)
{
    const char *NDDSHOME = ut_getenv(RTI_HOME);
    const char *NDDSPLATFORM = ut_getenv(RTI_PLATFORM);

    if (!NDDSHOME) {
        ut_error(RTI_HOME " is not defined");
        project->error = 1;
        return;
    }

    if (!NDDSPLATFORM) {
        ut_error(RTI_PLATFORM " is not defined");
        project->error = 1;
        return;
    }

    const char *idl = driver->get_attr_string(DRIVER_IDL_ATTR);
    if (idl && strlen(idl)) {
        char *ddsgen = ut_asprintf("%s/" RTI_DDSGEN_PATH, NDDSHOME);
        if (ut_file_test(ddsgen) != 1) {
            ut_error("did not find rtiddsgen in %s", ddsgen);
            project->error = 1;
            return;
        }

        ut_strbuf cmdbuf = UT_STRBUF_INIT;

        ut_strbuf_append(&cmdbuf, "%s %s/%s -language %s -d %s/src -replace", 
            ddsgen, project->path, idl, project->language, project->path);

        ut_strbuf_append(&cmdbuf, " -platform %s", NDDSPLATFORM);

        ut_strbuf_append(&cmdbuf, " -create typefiles");

        char *cmd = ut_strbuf_get(&cmdbuf);

        driver->exec(cmd);

        free(cmd);
        free(ddsgen);
    }
}

static
void move_to_include(
    bake_project *project,
    const char *file_pattern,
    const char *file_base)
{
    char *src_file = ut_asprintf("%s/src/%s%s", project->path, file_base, file_pattern);
    if (ut_file_test(src_file) == 1) {
        char *dst_file = ut_asprintf("%s/include/%s%s", project->path, file_base, file_pattern);
        ut_rename(src_file, dst_file);
        free(dst_file);
    }

    free(src_file);
}

static
void prebuild(
    bake_driver_api *driver,
    bake_config *config,
    bake_project *project)
{
    if (driver->get_attr_bool(DRIVER_PUBLIC_HEADERS_ATTR)) {
        char *idl_file = driver->get_attr_string(DRIVER_IDL_ATTR);
        if (idl_file && strlen(idl_file)) {
            char *idl_base = ut_strdup(idl_file);
            char *ext = strrchr(idl_base, '.');
            if (ext) {
                ext[0] = '\0';
            }

            move_to_include(project, RTI_IDL_HEADER, idl_base);
            move_to_include(project, RTI_IDL_PLUGIN_HEADER, idl_base);
            move_to_include(project, RTI_IDL_SUPPORT_HEADER, idl_base);
            
            free(idl_base);

            /* When headers are moved to 'include', add 'include' to list of 
             * includes of lang.c, so headers in generated files are found */
            bake_driver *c_driver = driver->lookup_driver("lang.c");
            if (c_driver) {
                bake_driver *current = driver->set_driver(c_driver);
                driver->set_attr_array("include", "include");
                driver->set_driver(current);
            }
        }
    }
}

BAKE_RTI_CONNEXT_EXPORT
int bakemain(bake_driver_api *driver) 
{
    ut_init("bake.test");

    /* Initialize new projects to add correct dependency */
    driver->init(init);

    /* Generate code based on configuration in project.json */
    driver->generate(generate);

    /* Move generated files to correct locations before starting build */
    driver->prebuild(prebuild);

    return 0;
}
