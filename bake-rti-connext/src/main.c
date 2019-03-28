#include <bake>

#include <include/connext.h>

static
void generate(
    bake_driver_api *driver,
    bake_config *config,
    bake_project *project)
{
    const char *NDDSHOME = ut_getenv("NDDSHOME");
    const char *NDDSPLATFORM = ut_getenv("NDDSPLATFORM");

    if (!NDDSHOME) {
        ut_error("NDDSHOME is not defined");
        project->error = 1;
        return;
    }

    if (!NDDSPLATFORM) {
        ut_error("NDDSPLATFORM is not defined");
        project->error = 1;
        return;
    }

    const char *idl = driver->get_attr_string("idl");
    if (idl) {
        char *ddsgen = ut_asprintf("%s/bin/rtiddsgen", NDDSHOME);
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
void init(
    bake_driver_api *driver,
    bake_config *config,
    bake_project *project)
{
    driver->use("rti.connext");
}

BAKE_RTI_CONNEXT_EXPORT
int bakemain(bake_driver_api *driver) 
{
    ut_init("bake.test");

    /* Generate code based on configuration in project.json */
    driver->generate(generate);

    /* Initialize new projects to add correct dependency */
    driver->init(init);

    return 0;
}
