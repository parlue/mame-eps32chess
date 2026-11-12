"""Check bank overlay reversibility; optionally emit actual mutator bodies for C++ testing."""
import argparse
from pathlib import Path
import unittest
from prepare_flat_banks import transform, SIGNATURES

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL = (ROOT / 'ref/mame0220-src/src/emu/emumem.cpp').read_text(encoding='utf-8')


class BankOverlayTests(unittest.TestCase):
    def test_exact_reversal_on_both_supported_inputs(self):
        for p in ('ref/mame0220-src/src/emu/emumem.cpp',
                  'esp32s3/components/mame0220/generated/emumem_roots_s3.cpp'):
            with self.subTest(path=p):
                original = (ROOT / p).read_text(encoding='utf-8')
                changed = transform(original)
                self.assertEqual(changed.count('\ts3_flat_bank_changed();'),4)
                # transform itself also asserts byte-exact reversal.
                self.assertEqual(changed.count('extern "C" void s3_flat_bank_changed();'),1)
                self.assertEqual(changed.count('register_postload(save_prepost_delegate([] { s3_flat_bank_changed(); }'),1)

    def test_already_patched_and_changed_upstream_rejected(self):
        with self.assertRaises(AssertionError): transform(transform(ORIGINAL))
        with self.assertRaises(AssertionError): transform(ORIGINAL.replace('m_curentry = entrynum;', 'm_curentry=entrynum;'))


def emit_cpp(path):
    changed = transform(ORIGINAL)
    bodies = []
    for signature in SIGNATURES:
        start = changed.index(signature)
        stop = changed.index('\n}\n',start)+3
        bodies.append(changed[start:stop])
    prefix = r'''
#include <cassert>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdio>
using u8 = unsigned char;
using offs_t = unsigned;
struct emu_fatalerror : std::runtime_error {
    template<class... T> emu_fatalerror(const char *s, T...) : std::runtime_error(s) {}
};
std::function<void()> before_mutation;
bool valid = true;
int notifications = 0;
extern "C" void s3_flat_bank_changed() {
    if (before_mutation) before_mutation();
    ++notifications; valid = false;
}
struct memory_bank {
    std::vector<u8 *> m_entries;
    int m_curentry = 0;
    bool m_anonymous = false;
    std::string m_tag = "test";
    std::vector<std::function<void(void *)>> m_alloc_notifier;
    void set_base(void *base);
    void set_entry(int entrynum);
    void configure_entry(int entrynum, void *base);
    void configure_entries(int startentry,int numentries,void *base,offs_t stride);
};
'''
    main = r'''
int main() {
    u8 a[64]{}, b[64]{};
    memory_bank bank;
    before_mutation = [&] { assert(bank.m_entries.empty()); };
    bank.m_alloc_notifier.push_back([&](void *p) { assert(!valid && p == a); });
    bank.set_base(a);
    assert(!valid && bank.m_entries[0] == a && bank.m_alloc_notifier.empty());
    valid=true; before_mutation=[&]{assert(bank.m_entries[0] == a);};
    bank.configure_entry(0,b);
    assert(!valid && bank.m_entries[0] == b);
    valid=true; before_mutation=[&]{assert(bank.m_entries.size() == 1);};
    bank.configure_entries(1,2,a,16);
    assert(!valid && bank.m_entries[1] == a && bank.m_entries[2] == a+16);
    valid=true; before_mutation=[&]{assert(bank.m_curentry == 0);};
    bank.set_entry(2);
    assert(!valid && bank.m_curentry == 2);
    before_mutation={};
    const auto old = bank.m_entries;
    const int count = notifications;
    auto rejected = [&](auto action) {
        valid=true; bool caught=false;
        try { action(); } catch(const emu_fatalerror &) { caught=true; }
        assert(caught && valid && notifications == count && bank.m_entries == old && bank.m_curentry == 2);
    };
    rejected([&]{bank.set_base(nullptr);});
    rejected([&]{bank.configure_entry(-1,a);});
    rejected([&]{bank.set_entry(-1);});
    rejected([&]{bank.set_entry(99);});
    bank.m_anonymous=true;
    rejected([&]{bank.set_entry(0);});
    bank.m_anonymous=false;
    bank.m_entries[1]=nullptr;
    bool caught=false;
    try { bank.set_entry(1); } catch(const emu_fatalerror &) { caught=true; }
    assert(caught && valid && notifications == count && bank.m_curentry == 2);
    std::puts("PASS: actual four upstream mutators invalidate before writes/callbacks; validation exceptions preserve state");
}
'''
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(prefix+'\n'.join(bodies)+main,encoding='utf-8')


if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--emit-cpp',type=Path)
    a=p.parse_args()
    suite=unittest.defaultTestLoader.loadTestsFromTestCase(BankOverlayTests)
    result=unittest.TextTestRunner(verbosity=2).run(suite)
    if not result.wasSuccessful(): raise SystemExit(1)
    if a.emit_cpp: emit_cpp(a.emit_cpp)
