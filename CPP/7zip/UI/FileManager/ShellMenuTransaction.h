// ShellMenuTransaction.h

#ifndef ZIP7_INC_SHELL_MENU_TRANSACTION_H
#define ZIP7_INC_SHELL_MENU_TRANSACTION_H

namespace NShellMenuTransaction {

enum EPart { kModern, kFolders, kClassic, kNumParts };

// Install the replacement before removing the existing menu. Set() can fail
// after a partial write, so restore the failed operation as well as earlier ones.
// The backend reports both the original failure and any rollback failures.
template <class T>
bool Apply(T &backend, const unsigned *before, const unsigned *after)
{
  const EPart order[kNumParts] = {
      after[kModern] ? kModern : kFolders,
      after[kModern] ? kFolders : kClassic,
      after[kModern] ? kClassic : kModern };
  for (unsigned i = 0; i < kNumParts; i++)
  {
    const unsigned part = order[i];
    if (before[part] == after[part])
      continue;
    if (backend.Set(part, after[part], false))
      continue;
    for (unsigned j = i + 1; j != 0;)
    {
      const unsigned restorePart = order[--j];
      if (before[restorePart] != after[restorePart])
        backend.Set(restorePart, before[restorePart], true);
    }
    return false;
  }
  return true;
}

}

#endif
