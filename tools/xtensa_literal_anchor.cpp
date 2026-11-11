// Link before the probe to force the option strings to be merged elsewhere.
extern "C" const char *anchor(int key)
{
    return key == 0 ? "cfg_directory" : key == 1 ? "ctrlr" : "ctrlrpath";
}
