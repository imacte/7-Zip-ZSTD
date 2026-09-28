// Build with: cl /nologo /EHsc /W4 /WX tests/shell-menu-transaction.cpp
// This test uses an in-memory backend; it never touches Windows registrations.
#include <assert.h>
#include <stdio.h>

#include "../CPP/7zip/UI/FileManager/ShellMenuTransaction.h"

using namespace NShellMenuTransaction;

struct CBackend
{
  unsigned State[kNumParts];
  int FailPart;
  bool PartialFailure;
  int FailRestorePart;
  unsigned Calls;
  unsigned Restores;

  CBackend(const unsigned *initial, int fail, bool partial):
      FailPart(fail), PartialFailure(partial), FailRestorePart(-1), Calls(0), Restores(0)
  {
    for (unsigned i = 0; i < kNumParts; i++)
      State[i] = initial[i];
  }

  bool Set(unsigned part, unsigned value, bool restoring)
  {
    if (restoring)
    {
      Restores++;
      if ((int)part == FailRestorePart)
        return false;
    }
    else
    {
      Calls++;
      if ((int)part == FailPart)
      {
        if (PartialFailure)
          State[part] = value;
        return false;
      }
      // A replacement must exist before the previous menu is removed.
      if (part == kClassic && value == 0 && State[kFolders] != 0)
        assert(State[kModern] != 0);
    }
    State[part] = value;
    return true;
  }
};

int main()
{
  unsigned cases = 0;
  // Include partially configured folder/native/alternate registrations.
  for (unsigned modern = 0; modern < 2; modern++)
  for (unsigned folders = 0; folders < 4; folders++)
  for (unsigned classic = 0; classic < 4; classic++)
  for (unsigned target = 0; target < 4; target++)
  {
    const unsigned before[kNumParts] = { modern, folders, classic };
    const unsigned after[kNumParts] = {
        (target & 1) ? 1u : 0u, (target & 1) ? 3u : 0u,
        (target & 2) ? 3u : 0u };
    for (int failure = -1; failure < kNumParts; failure++)
    for (unsigned partial = 0; partial < 2; partial++)
    {
      CBackend backend(before, failure, partial != 0);
      const bool success = Apply(backend, before, after);
      const bool expectFailure = failure >= 0 && before[failure] != after[failure];
      assert(success != expectFailure);
      const unsigned *expected = success ? after : before;
      for (unsigned i = 0; i < kNumParts; i++)
        assert(backend.State[i] == expected[i]);
      if (!success)
        assert(backend.Restores > 0);
      cases++;
    }
  }
  // Continue restoring earlier operations even if one rollback itself fails.
  const unsigned before[kNumParts] = { 0, 0, 3 };
  const unsigned after[kNumParts] = { 1, 3, 0 };
  CBackend backend(before, kClassic, true);
  backend.FailRestorePart = kClassic;
  assert(!Apply(backend, before, after));
  assert(backend.Restores == 3);
  assert(backend.State[kModern] == 0 && backend.State[kFolders] == 0);
  printf("PASS: %u menu transitions and rollback failure recovery\n", cases);
  return 0;
}
