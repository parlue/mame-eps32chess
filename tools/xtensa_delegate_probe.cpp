// Inspect the actual compiler's pointer-to-member representation. MAME's
// internal delegate decoder assumes the virtual flag is in the function word.
class DelegateAbiProbe {
public:
    virtual void virtual_method();
    void direct_method();
};
using ProbeMethod=void (DelegateAbiProbe::*)();
ProbeMethod probe_virtual=&DelegateAbiProbe::virtual_method;
ProbeMethod probe_direct=&DelegateAbiProbe::direct_method;
