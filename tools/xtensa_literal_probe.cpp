// Freestanding reproduction probe for literal relocation/merging. No ESP API.
extern "C" __attribute__((noinline)) const char *identity(const char *text)
{
    asm volatile("" : "+r"(text));
    return text;
}
extern "C" __attribute__((noinline)) void consume(const char *begin, const char *end)
{
    asm volatile("" : "+r"(begin), "+r"(end));
}
extern "C" const char *probe(int key)
{
    if (key == 7) {
        constexpr const char *format = "Could not load controller file %s.cfg";
        consume(format, format + __builtin_strlen(format) + 1);
    }
    switch (key) {
    case 0: return identity("ctrlr");
    case 1: return identity("ctrlrpath");
    case 2: return identity("cfg_directory");
    default: return identity("default.cfg");
    }
}
