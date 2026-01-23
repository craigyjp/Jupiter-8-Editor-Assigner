#include <CircularBuffer.hpp>

#define TOTALCHARS 63

const char CHARACTERS[TOTALCHARS] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', ' ', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};
int charIndex = 0;
char currentCharacter = 0;
String renamedPatch = "";
int upperPatchIndex = 0;
int lowerPatchIndex = 0;

struct PatchNoAndName
{
  int patchNo;
  String patchName;
};

CircularBuffer<PatchNoAndName, PATCHES_LIMIT> patches;

size_t readField(File *file, char *str, size_t size, const char *delim)
{
  char ch;
  size_t n = 0;
  while ((n + 1) < size && file->read(&ch, 1) == 1)
  {
    // Delete CR.
    if (ch == '\r')
    {
      continue;
    }
    str[n++] = ch;
    if (strchr(delim, ch))
    {
      break;
    }
  }
  str[n] = '\0';
  return n;
}

static inline void ensureDir(const String &dir) {
  if (!SD.exists(dir.c_str())) SD.mkdir(dir.c_str());
}

static inline String bankBaseDir(uint8_t bank) {
  char buf[20];
  snprintf(buf, sizeof(buf), "/banks/b%02u", (unsigned)bank);
  return String(buf);
}

static inline void ensureJP8BankFolders(uint8_t bank) {
  ensureDir("/banks");
  const String base = bankBaseDir(bank);
  ensureDir(base);
  ensureDir(base + "/patches");
  ensureDir(base + "/performances");
}

// PATCH files are named: "11".."88"
static inline String patchPathFromRC(uint8_t rc) {
  const String base = bankBaseDir(activeBank);
  return base + "/patches/" + String(rc);
}

// PERF files are named: "perf11".."perf88"
static inline String perfPathFromRC(uint8_t rc) {
  const String base = bankBaseDir(activeBank);
  return base + "/performances/perf" + String(rc);
}

void recallPatchData(File patchFile, String data[])
{
  //Read patch data from file and set current patch parameters
  size_t n;     // Length of returned field with delimiter.
  char str[20]; // Must hold longest field with delimiter and zero byte.
  int i = 0;
  while (patchFile.available() && i < NO_OF_PARAMS)
  {
    n = readField(&patchFile, str, sizeof(str), ",\n");
    // done if Error or at EOF.
    if (n == 0)
      break;
    // Print the type of delimiter.
    if (str[n - 1] == ',' || str[n - 1] == '\n')
    {
      // Remove the delimiter.
      str[n - 1] = 0;
    }
    else
    {
      // At eof, too long, or read error.  Too long is error.
      Serial.print(patchFile.available() ? F("error: ") : F("eof:   "));
    }
    // Print the field.
    //    Serial.print(i);
    //    Serial.print(" - ");
    //    Serial.println(str);
    data[i++] = String(str);
  }
}

int compare(const void *a, const void *b) {
  return ((PatchNoAndName*)a)->patchNo - ((PatchNoAndName*)b)->patchNo;
}

void loadPatches() {
  patches.clear();

  const String patchesDir = bankBaseDir(activeBank) + "/patches";
  File dir = SD.open(patchesDir.c_str());
  if (!dir || !dir.isDirectory()) {
    Serial.print("Failed to open patches dir: ");
    Serial.println(patchesDir);
    return;
  }

  while (true) {
    File patchFile = dir.openNextFile();
    if (!patchFile) break;

    if (patchFile.isDirectory()) {
      patchFile.close();
      continue;
    }

    const char *fname = patchFile.name(); // "11".."88"
    if (!fname || strlen(fname) != 2 ||
        fname[0] < '1' || fname[0] > '8' ||
        fname[1] < '1' || fname[1] > '8') {
      patchFile.close();
      continue;
    }

    String data[NO_OF_PARAMS];
    recallPatchData(patchFile, data);

    const uint8_t rc = (uint8_t)((fname[0] - '0') * 10 + (fname[1] - '0'));
    patches.push(PatchNoAndName{ (int)rc, data[0] });

    patchFile.close();
  }

  dir.close();
}

void savePatch(const char *patchNo, String patchData) {
  if (!patchNo) return;

  // Expect "11".."88"
  if (strlen(patchNo) != 2 ||
      patchNo[0] < '1' || patchNo[0] > '8' ||
      patchNo[1] < '1' || patchNo[1] > '8') {
    Serial.print("Invalid patchNo: ");
    Serial.println(patchNo);
    return;
  }

  const uint8_t rc = (uint8_t)atoi(patchNo);
  const String path = patchPathFromRC(rc);   // /banks/bXX/patches/11

  ensureJP8BankFolders(activeBank);          // safety

  if (SD.exists(path.c_str())) SD.remove(path.c_str());

  File patchFile = SD.open(path.c_str(), FILE_WRITE);
  if (patchFile) {
    patchFile.println(patchData);
    patchFile.close();
  } else {
    Serial.print("Error writing Patch file: ");
    Serial.println(path);
  }
}

void savePatch(const char *patchNo, String patchData[])
{
  String dataString = patchData[0];
  for (int i = 1; i < NO_OF_PARAMS; i++)
  {
    dataString = dataString + "," + patchData[i];
  }
  savePatch(patchNo, dataString);
}

