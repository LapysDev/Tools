/* ...
        --- EXAMPLE (Lapys) -> `batch-renamer -format="File #%lu (%s)"`
        --- NOTE (Lapys) -> Default to the current directory if no directory argument is specified.
*/
/* Import */
#include <math.h> // Mathematics
#include <stdarg.h> // Standard Arguments
#include <stdint.h> // Standard Integer
#include <stdio.h> // Standard Input/ Output
#include <stdlib.h> // Standard Library
#include <string.h> // String

/* Definition > ... */
#if defined(__NT__) || defined(__TOS_WIN__) || defined(_WIN16) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN32_WCE) || defined(_WIN64) || defined(__WINDOWS__)
#  define PLATFORM__IS__POSIX false
#  define PLATFORM__IS__UNKNOWN false

#  define PLATFORM__IS_WINDOWS true
#elif ((defined(__APPLE__) && defined(__MACH__) || defined(__gnu_linux__) || defined(linux) || defined(__linux) || defined(__linux__)) || (defined(unix) || defined(__unix) || defined(__unix__)))
#  define PLATFORM__IS__POSIX true
#  define PLATFORM__IS__UNKNOWN false

#  define PLATFORM__IS_WINDOWS false
#else
#  define PLATFORM__IS__POSIX false
#  define PLATFORM__IS__UNKNOWN true

#  define PLATFORM__IS_WINDOWS false
#endif

typedef unsigned char byte_t;
#if defined(__cplusplus) && __cplusplus >= 201103L
    typedef long long wide_t;
    typedef unsigned long long uwide_t;
#else
    typedef long uwide_t;
    typedef unsigned long uwide_t;
#endif

void configure(void) throw();
void configure(char[], size_t) throw();
inline void configure(char const* const argument, size_t const length) throw() { configure((char*) argument, length); }
void correctDirectory(char[]) throw();
void debug(void) throw();

/* Global > (Configurations, Entries, Entry Count, Program (Buffer ..., Path)) */
struct configurations_t { // NOTE (Lapys) ->
    // Collection of entries to ignore.
    char **filters = NULL;
    size_t filterCount = 0u;

    // C format-specifier for manipulating the substitute file names.
    char *format = NULL;

    // Terminate the program early if this is truthy.
    bool helpGuideRequested = false;

    // Current (index of) file entry.
    uwide_t index = 0u;
    wide_t end = -1, start = -1;

    // Maximum & minimum length of file substitute file name.
    // - When set to `-1`, they are based on the longest & shortest entry names.
    short maximumLength = -1;
    short minimumLength = -1;

    // Order of renaming the indexes.
    enum category_t {ACCESS_DATE = 0x1, MODIFICATION_DATE = 0x2, NAME = 0x4, REVERSE = 0x8, STATUSCHANGE_DATE = 0x10} order = (category_t) 0x0;

    // Leading/ trailing text for evening out the length of all file names.
    char *pad = NULL;
    enum paddir_t {LEADING = 0x1, TRAILING = 0x2} padDirection = (paddir_t) 0x0;
    bool padIndexOnly = true;

    // Directory for indexing entries.
    char *path = NULL;

    // Forcefully rename files with the same name as their substitute.
    bool resolveEntryConflicts = true;
} configurations = {};

struct fileinfo_t {
    char *const name;
    uwide_t const timeSinceLastAccess;
    uwide_t const timeSinceLastModification;
    uwide_t const timeSinceLastStatusChange;
    unsigned long const size;

    static size_t ENTRY_MAX;
} *entries = NULL;
size_t entryCount = 0u;
size_t fileinfo_t::ENTRY_MAX = {};

void *programBuffer = NULL;
size_t programBufferSize = 0u;
char *programPath = NULL;

/* Phase */
    /* Initiate */
    bool Initiate(size_t const ENTRY_MAX) throw() {
        // Constant > ... Size
        size_t const configurationFormatSize = 0xFFF * sizeof(char); // NOTE (Lapys) -> Minimum character length producible per conversion from `printf(...)`.
        size_t const configurationPadSize = FILENAME_MAX * sizeof(char);
        size_t const configurationPathSize = ENTRY_MAX * sizeof(char);
        size_t const programPathSize = ENTRY_MAX * sizeof(char);

        // Modification > ...
        fileinfo_t::ENTRY_MAX = ENTRY_MAX;

        // Update > Program Buffer ... --- NOTE (Lapys) -> Setup the program buffer.
        programBufferSize = configurationFormatSize + configurationPadSize + configurationPathSize + programPathSize;
        programBuffer = ::malloc(programBufferSize);

        // Logic > Return --- NOTE (Lapys) -> Allocate the memory for the program's heaped data.
        if (NULL == programBuffer) return false;
        /* ... */ {
            configurations.format = (char*) ((byte_t*&) programBuffer += 0x0);
            configurations.pad = (char*) ((byte_t*&) programBuffer += configurationFormatSize);
            configurations.path = (char*) ((byte_t*&) programBuffer += configurationPadSize);

            programPath = (char*) ((byte_t*&) programBuffer += configurationPathSize);
            configurations.filters = (char**) ((byte_t*&) programBuffer += programPathSize);
        } ((byte_t*&) programBuffer) -= programBufferSize;

        /* Update > Program Buffer
            --- NOTE (Lapys) -> Null-terminate the program's strings.
            --- WARN (Lapys) -> Resized memory may not be guaranteed to be zeroed.
        */
        ::memset(programBuffer, 0x0, programBufferSize);

        // Return
        return true;
    }

    /* Terminate */
    int Terminate(void) throw() { ::free(programBuffer); ::fflush(stdout); ::exit(EXIT_SUCCESS); return EXIT_SUCCESS; }
    int Terminate(char const message[], ...) throw() { va_list arguments; ::fputs("\r\n", stderr); va_start(arguments, message); ::vfprintf(stderr, message, arguments); va_end(arguments); ::fputc('\0', stderr); ::free(programBuffer); ::fflush(stdout); ::exit(EXIT_FAILURE); return EXIT_FAILURE; }

/* Function */
    // Configure --- NOTE (Lapys) ->
    // : `argument` may as well represent read-only temporary memory.
    void configure(char argument[], size_t length) throw() {
        // Initialization > Invoked
        static bool invoked = false;

        // Logic > ...
        if (false == invoked) {
            /* Update > Program Path ... */ {
                ::strncpy(programPath, argument, length);
                *(programPath + length) = '\0';
                correctDirectory(programPath);
            } invoked = true;
        }

        else {
            // Initialization > (..., Options) --- REDACT (Lapys)
            enum {UNPAIRED = false, PAIRED = true};
            static struct option_t {
                bool const paired; bool parsed;
                char const *const name; char const *value;

                constexpr inline option_t(char const name[], bool const paired) : paired{paired}, parsed{false}, name{name}, value{NULL} {}
            } options[] = { // NOTE (Lapys) -> Be explicit about the option being long (`--`) or short (`-`).
                {"--end", PAIRED}, {"--start", PAIRED},
                {"--filter", PAIRED},
                {"--format", PAIRED},
                {"--help", UNPAIRED},
                {"--max", PAIRED}, {"--min", PAIRED},
                {"--Oaccess", UNPAIRED}, {"--Omodify", UNPAIRED}, {"--Oname", UNPAIRED}, {"--Oreverse", UNPAIRED}, {"--Ostatus", UNPAIRED},
                {"--pad", PAIRED}, {"--Pindex", UNPAIRED}, {"--Pleft", UNPAIRED}, {"--Pright", UNPAIRED},
                {"--path", PAIRED},
                {"--strict", UNPAIRED}
            };

            /* [Parse Argument] */ {
                // Loop
                for (option_t *iterator = options, *const end = iterator + (sizeof(options) / sizeof(option_t)); end != iterator; ++iterator) {
                    // Constant > Option Name (Length)
                    char const *const optionName = iterator -> name;
                    unsigned char const optionNameLength = ::strlen(optionName);

                    // Logic
                    if (length >= optionNameLength && 0 == ::strncmp(argument, optionName, optionNameLength)) {
                        // Logic --- NOTE (Lapys) ->
                        if (iterator -> paired) { // `key=value` command-line option; `argument` & `length` coincide to the command-line option value.
                            // Update > (Argument, Length)
                            argument += optionNameLength;
                            length -= optionNameLength;

                            // Loop > Logic > ... --- NOTE (Lapys) -> Assess the `value` of the `key`.
                            for (char *argumentIterator = argument, *const argumentEnd = argument + length; argumentEnd != argumentIterator; ++argumentIterator)
                            switch (*argumentIterator) {
                                case ' ': break;
                                case '=': length -= argumentIterator - argument; argument = argumentIterator + 1; goto update; break;
                                default: length = optionNameLength; argument -= optionNameLength; goto terminate__parsingArguments; break;
                            }

                            // Terminate --- ERROR (Lapys)
                            Terminate("[Error]: Unable to startup program; `%s` command-line option requires a value", optionName);

                            // [Update]
                            update: {
                                // Modification > [option_t] > Value; ...
                                iterator -> value = argument;
                                goto assert;
                            }
                        }

                        else if (length == optionNameLength) { // `key` command-line option.
                            // Modification > [option_t] > Value; ...
                            iterator -> value = (char*) 0x1;
                            goto assert;
                        }

                        else goto terminate__parsingArguments;

                        // [Assert]
                        assert: {
                            // Logic > ...
                            if (iterator -> parsed) Terminate("[Error]: Unable to startup program; `%s` must only be specified once", optionName); // ERROR (Lapys)
                            else iterator -> parsed = true;
                        }

                        // [Terminate] --- ERROR (Lapys)
                        continue;
                        terminate__parsingArguments: Terminate("[Error]: Unable to startup program; `%.*s` is not a recognized command-line option", length, argument);
                    }

                    else { // CHECKPOINT (Lapys) -> Presumably shorthand for the `--path` command-line option.
                        iterator -> value = argument;
                    }
                }
            }

            /* [Parse Configurations] */ {
                // Loop > Logic
                for (option_t *iterator = options, *const end = iterator + (sizeof(options) / sizeof(option_t)); end != iterator; ++iterator)
                if (NULL != iterator -> value) {
                    // ...; Initialization > Configuration Values
                    enum value_t {number, string};
                    struct valueinfo_t {
                        value_t type;
                        void *value;

                        constexpr inline valueinfo_t() : type{(value_t) 0x0}, value{NULL} {}
                        constexpr inline valueinfo_t(void *const value, value_t const type) : type{type}, value{value} {}
                    } configurationValues[2] = {}; // NOTE (Lapys) -> Maximum of two `valueinfo_t` for configuration per command-line option for now.

                    // Logic
                    if (0 == ::strcmp("--end", iterator -> name)) configurationValues[0] = valueinfo_t {&configurations.end, number};
                    else if (0 == ::strcmp("--format", iterator -> name)) configurationValues[0] = valueinfo_t {configurations.format, string};
                    else if (0 == ::strcmp("--max", iterator -> name)) configurationValues[0] = valueinfo_t {&configurations.maximumLength, number};
                    else if (0 == ::strcmp("--min", iterator -> name)) configurationValues[0] = valueinfo_t {&configurations.minimumLength, number};
                    else if (0 == ::strcmp("--pad", iterator -> name)) configurationValues[0] = valueinfo_t {configurations.pad, string};
                    else if (0 == ::strcmp("--path", iterator -> name)) configurationValues[0] = valueinfo_t {configurations.path, string};
                    else if (0 == ::strcmp("--start", iterator -> name)) {
                        configurationValues[0] = valueinfo_t {&configurations.index, number};
                        configurationValues[1] = valueinfo_t {&configurations.start, number};
                    }

                    else if (0 == ::strcmp("--help", iterator -> name)) configurations.helpGuideRequested = true;
                    else if (0 == ::strcmp("--Oaccess", iterator -> name)) (int&) configurations.order |= configurations_t::category_t::ACCESS_DATE;
                    else if (0 == ::strcmp("--Omodify", iterator -> name)) (int&) configurations.order |= configurations_t::category_t::MODIFICATION_DATE;
                    else if (0 == ::strcmp("--Oname", iterator -> name)) (int&) configurations.order |= configurations_t::category_t::NAME;
                    else if (0 == ::strcmp("--Oreverse", iterator -> name)) (int&) configurations.order |= configurations_t::category_t::REVERSE;
                    else if (0 == ::strcmp("--Ostatus", iterator -> name)) (int&) configurations.order |= configurations_t::category_t::STATUSCHANGE_DATE;
                    else if (0 == ::strcmp("--Pindex", iterator -> name)) configurations.padIndexOnly = true;
                    else if (0 == ::strcmp("--Pleft", iterator -> name)) (int&) configurations.padDirection |= configurations_t::paddir_t::LEADING;
                    else if (0 == ::strcmp("--Pright", iterator -> name)) (int&) configurations.padDirection |= configurations_t::paddir_t::TRAILING;
                    else if (0 == ::strcmp("--strict", iterator -> name)) configurations.resolveEntryConflicts = true;
                    else if (0 == ::strcmp("--filter", iterator -> name)) {
                        // Constant > (Buffer, ... Size)
                        size_t const filterSize = fileinfo_t::ENTRY_MAX * sizeof(char);
                        void *buffer = ::realloc(programBuffer, programBufferSize += filterSize + sizeof(char*));

                        // Logic
                        if (NULL == buffer) goto terminate__parsingConfigurations;
                        else {
                            programBuffer = buffer;
                            buffer = programBufferSize + (byte_t*) programBuffer;

                            ::strncpy((char*) ((byte_t*&) buffer -= filterSize), argument, length);
                            *(configurations.filters + configurations.filterCount) = (char*) buffer;
                            *(length + *(configurations.filters + configurations.filterCount)) = '\0';

                            for (char **start = configurations.filters - 1, **iterator = start + configurations.filterCount; iterator != start; --iterator) {
                                ::strncpy((char*) ((byte_t*&) buffer -= filterSize), *iterator, filterSize);
                                *iterator = (char*) buffer;
                            }

                            ++configurations.filterCount;
                        }

                        continue;
                        terminate__parsingConfigurations: Terminate("[Error]: Unable to startup program; Not enough resources to parse command-line arguments", length, argument);
                    }
                }
            }

            /* [...] --- NOTE (Lapys) -> Prepare the `options` for the next `argument`. */ {
                // Loop > Modification > [option_t] > Value
                for (option_t *const end = options + (sizeof(options) / sizeof(option_t)), *iterator = end; end != iterator; ++iterator)
                iterator -> value = NULL;
            }
        }
    }

    // : Set default values for (some of) the configurations
    void configure(void) throw() {
        // [Format] ...
        if ('\0' == *configurations.format)
        ::strncpy(configurations.format, "%lu", 4u);

        // [Pad] ... --- NOTE (Lapys) -> Not required.
        if ('\0' == *configurations.pad) {
            configurations.pad = NULL;
            configurations.padDirection = (configurations_t::paddir_t) 0x0;
        }

        else if (0x0 == configurations.padDirection)
        configurations.padDirection = configurations_t::paddir_t::LEADING;

        // [Path] ...
        if ('\0' == *configurations.path) {
            #if PLATFORM__IS_WINDOWS
                ::strncpy(configurations.path, "./*", 4u);
            #else
                ::strncpy(configurations.path, "./", 3u);
            #endif
        }
    }

    // Correct Directory
    void correctDirectory(char path[]) throw() { (void) path; }

    // Debug
    inline void debug(void) throw() {
        // [Configurations]
        ::fprintf(stdout, "[CONFIGURATIONS]: {"); {
            // [Filters]
            ::fprintf(stdout, "\r\n\t[FILTERS (%zu)]: [", configurations.filterCount); {
                for (char **iterator = configurations.filters, **end = iterator + configurations.filterCount; end != iterator; ++iterator)
                ::fprintf(stdout, "\"%s\"%s", *iterator, end - 1 == iterator ? "" : ", ");
            } ::putchar(']');

            // [...]
            ::fprintf(stdout, "\r\n\t[FORMAT]: \"%s\"", configurations.format);
            ::fprintf(stdout, "\r\n\t[HELP GUIDE REQUESTED]: %s", configurations.helpGuideRequested ? "true" : "false");
            ::fprintf(stdout, "\r\n\t[INDEX]: %lu {%li -> %li}", (unsigned long) configurations.index, (signed long) configurations.start, (signed long) configurations.end);
            ::fprintf(stdout, "\r\n\t[LENGTH]: {%i -> %i}", configurations.minimumLength, configurations.maximumLength);

            // [Order]
            ::fprintf(stdout, "\r\n\t[ORDER BY]: %s%s",
                configurations.order & configurations_t::category_t::ACCESS_DATE ? "RECENT ACCESS" :
                configurations.order & configurations_t::category_t::MODIFICATION_DATE ? "RECENT MODIFICATION" :
                configurations.order & configurations_t::category_t::STATUSCHANGE_DATE ? "RECENT STATUS CHANGE" :
                "(any)",

                configurations.order & configurations_t::category_t::REVERSE ? " (REVERSE)" : ""
            );

            // [Pad]
            ::fputs("\r\n", stdout);
            ::fprintf(stdout, "\r\n\t[PAD]: \"%s\"", NULL == configurations.pad ? "(null)" : configurations.pad);
            ::fprintf(stdout, "\r\n\t[PAD DIRECTION]: %s",
                configurations.padDirection == (configurations_t::paddir_t::LEADING | configurations_t::paddir_t::TRAILING) ? "BOTH (LEADING | TRAILING)" :
                configurations.padDirection & configurations_t::paddir_t::LEADING ? "LEADING" :
                configurations.padDirection & configurations_t::paddir_t::TRAILING ? "TRAILING" :
                "(null)"
            );
            ::fprintf(stdout, "\r\n\t[PAD INDEXES ONLY]: %s", configurations.padIndexOnly ? "true" : "false");

            // [...]
            ::fputs("\r\n", stdout);
            ::fprintf(stdout, "\r\n\t[PATH]: \"%s\"", configurations.path);
            ::fprintf(stdout, "\r\n\t[STRICT RESOLUTION]: %s", configurations.resolveEntryConflicts ? "true" : "false");
        } ::fprintf(stdout, "\r\n}");

        // [Program]
        ::fputs("\r\n\n", stdout);
        ::fprintf(stdout, "[PROGRAM]: {"); {
            size_t const size = (programBufferSize * sizeof(byte_t)) / sizeof(char);

            ::fprintf(stdout, "\r\n\t[PATH]: \"%s\"", programPath);
            ::fprintf(stdout, "\r\n\t[BUFFER (%zu bytes)]: `%.*s`", programBufferSize, (int) (size > 128u ? 128u : size), (char*) programBuffer);
        } ::fprintf(stdout, "\r\n}");

        // [Entries]
        ::fputs("\r\n\n", stdout);
        ::fprintf(stdout, "[ENTRIES]: ["); if (entryCount) {
            for (fileinfo_t *iterator = entries, *end = iterator + entryCount; end != iterator; ++iterator)
            ::fprintf(stdout, "\r\n\t\"%s\" {[SIZE]: %lu, [TIME SINCE]: {[ACCESS]: %lu, [MODIFICATION]: %lu, [STATUS CHANGE]: %lu}}%s",
                iterator -> name, iterator -> size, (unsigned long) iterator -> timeSinceLastAccess, (unsigned long) iterator -> timeSinceLastModification, (unsigned long) iterator -> timeSinceLastStatusChange,
                end - 1 == iterator ? "" : ", "
            );

            ::fprintf(stdout, "\r\n");
        } ::putchar(']');

        ::putchar('\0');
        ::fflush(stdout);
    }

/* Main */
#include <windows.h>
int main(int const, char const *const *const path) {
    if (false == Initiate(PATH_MAX)) Terminate("[Error]: Unable to startup program");
    else {
        configure(*path, ::strlen(*path));
        configure("--filter=audio.ogg", 18u);
        configure("--filter=batch.bat", 18u);
        configure("--filter=snail.png", 18u);
        configure();
        debug();
    }

    return Terminate();
}

// /* Logic */
// #if defined(__NT__) || defined(__TOS_WIN__) || defined(_WIN16) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN32_WCE) || defined(_WIN64) || defined(__WINDOWS__)
//     /* Import */
//     #ifdef q4_WCE // Input/ Output
//     #   include <io.h>
//     #endif
//     #include <stdint.h> // Standard Integers
//     #include <wchar.h> // Wide Characters -- Window
//     #include <windows.h> // Windows

//     /* Main */
//     int WinMain(HINSTANCE const, HINSTANCE const, LPSTR const, int const) {
//         // Initialization > Argument ... --- WARN (Lapys) -> `CommandLineToArgvA(...)` may not be available for some implementations.
//         int argumentCount;
//         LPWSTR *const arguments = ::CommandLineToArgvW(::GetCommandLineW(), &argumentCount);

//         // Logic > ...
//         if (NULL == arguments && 0 == argumentCount) { // ERROR (Lapys)
//             // Initialization > ...
//             DWORD const id = ::GetLastError();
//             WCHAR message[512] = {};

//             // ... Update > Message
//             // Terminate
//             if (0 != ::FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM, NULL, id, 0u, message, 512u, NULL)) *message = L'\0';
//             Terminate(
//                 "[Error]: Unable to startup program; Not enough resources to parse command-line arguments%s%ls"
//                 L'\0' == *message ? "" : ",\r\n\t",
//                 L'\0' == *message ? L"" : message
//             );
//         }
//         else {
//             // Initialization > Argument
//             // : Loop
//             LPSTR argument = NULL;
//             for (LPWSTR *iterator = arguments; (NULL != argument || arguments == iterator) && argumentCount != iterator - arguments; ++iterator) {
//                 // Constant > (Length, Size)
//                 int const length = (int) ::wcslen(*iterator);
//                 int const size = ::WideCharToMultiByte(CP_UTF8, 0x0, *iterator, length, NULL, 0, NULL, NULL);

//                 // Update > Argument --- NOTE (Lapys) -> Initially invoked with a `NULL` `argument`.
//                 // : Logic
//                 argument = (LPSTR) ::realloc(programBuffer, programBufferSize + size);
//                 if (NULL != argument) {
//                     // ... Configure
//                     programBuffer = argument;
//                     argument = programBufferSize + (byte_t*) programBuffer;
//                     ::WideCharToMultiByte(CP_UTF8, 0x0, *iterator, -1, argument, length, NULL, NULL);

//                     configure(argument, length);
//                 }
//             }

//             // ... Deletion
//             ::LocalFree(arguments);

//             if (NULL == argument) // ERROR (Lapys)
//             Terminate("[Error]: Unable to startup program; Not enough resources to parse command-line arguments");
//         }

//         debug();

//         // Return
//         return EXIT_SUCCESS;
//     }

// #else
//     /* Main */
//     int main(int const count, char const* const arguments[]) {
//         // ... Configure
//         for (int iterator = 0; count ^ iterator; ++iterator)
//         configure(arguments[iterator], ::strlen(arguments[iterator]));

//         debug();

//         // Return
//         return EXIT_SUCCESS;
//     }
// #endif
