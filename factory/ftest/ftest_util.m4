dnl $Id: ftest_util.m4,v 1.2 1997-09-24 07:28:21 schmidt Exp $
dnl
dnl ftest_util.m4 - m4 macros used by the factory test environment.
dnl
dnl "External" macro start with prefix `ftest', "internal" macros
dnl with prefix `_'.
dnl
dnl Note: mind the ';'!
dnl
dnl do not output anything of this library
divert(-1)


#
# - internal macros.
#

#
# _stripTWS() - strip trailing white space from $1.
#
define(`_stripTWS', `dnl
patsubst(`$1', `[ 	]*$')')

#
# _qstripTWS() - strip trailing white space from $1, return
#   quoted result.
#
define(`_qstripTWS', `dnl
ifelse(
  translit(`$1', ` 	'), `', ,
  `patsubst(`$1', `\(.*[^ 	]\)[ 	]*$', ``\1'')')')


#
# - external macros.
#

#
# ftestSetNameOfGame() - set name of game.
#
# $1: name of algorithm
# $2: usage of algorithm
#
# These are stored in the macros `ftestAlgorithm' and
# `ftestUsage', resp.  Leave notice on creator of this file.
#
define(`ftestSetNameOfGame', `dnl
define(`ftestAlgorithm',``$1'')dnl
define(`ftestUsage',``$2'')dnl
`/* This file was automatically generated by m4 using the ftest_util.m4 library */'')

#
# ftestPreprocInit() - initial preprocessor directives.
#
define(`ftestPreprocInit', `dnl
`#include <unistd.h>

#define TIMING
#include <timing.h>

#include <factory.h>

#include "ftest_util.h"
#include "ftest_io.h"
'')

#
# ftestGlobalInit() - global initialization.
#
define(`ftestGlobalInit', `dnl
`TIMING_DEFINE_PRINT( ftestTimer )'')

#
# ftestMainInit() - initialization in main().
#
define(`ftestMainInit', `dnl
`int optind = 0;
    ftestStatusT check = UndefinedResult;

    ftestSetName( argv[0], "'ftestAlgorithm`",
        'ftestUsage`);

    ftestSignalCatch()'')

#
# ftestMainExit() - clean up in main().
#
define(`ftestMainExit', `dnl
`return check'')

#
# ftestOutVar() - declare output variable.
#
# $1: type of output variable
# $2: name of output variable
#
# Stores type of variable in macro _ftestOutType_<name>.
#
define(`ftestOutVar', `dnl
define(`_ftestOutType_'_qstripTWS(`$2'), `$1')dnl
`$1 '_qstripTWS(`$2')')

#
# ftestInVar() - declare input variable.
#
# $1: type of input variable
# $2: name of input variable
#
# Stores type of variable in macro _ftestInType_<name>.
#
define(`ftestInVar', `dnl
define(`_ftestInType_'_qstripTWS(`$2'), `$1')dnl
`$1 '_qstripTWS(`$2')')

#
# ftestGetOpts() - read options.
#
define(`ftestGetOpts', `dnl
`ftestGetOpts( argc, argv, optind )'')

#
# ftestGetEnv() - read environment.
#
define(`ftestGetEnv', `dnl
`ftestGetEnv( argc, argv, optind );
    ftestPrintEnv()'')

#
# ftestGetInVar() - read variable from command line.
#
# $1: name of input variable.
# $2: default for optional command line arguments
#
define(`ftestGetInVar', `dnl
ifelse(`$#', `1',
  ``$1= ftestGet'_stripTWS(`_ftestInType_$1')`( argv[ optind++ ] )'',
  ``if ( argv[ optind ] )
	$1 = ftestGet'_stripTWS(`_ftestInType_$1')`( argv[ optind++ ] );
    else
	$1 = '_qstripTWS(`$2')')')

#
# ftestSetEnv() - set factory environment.
#
define(`ftestSetEnv', `dnl
`ftestSetEnv();
    ftestPrintEnv()'')

#
# ftestRun() - run test.
#
# $1: code to execute
#
define(`ftestRun', `dnl
`if ( ftestAlarm )
	alarm( ftestAlarm );
    while ( ftestCircle > 0 ) {
	TIMING_START(ftestTimer);
	$1
	TIMING_END(ftestTimer);
	ftestCircle--;
    }'')

#
# ftestCheck() - run check.
#
# $1: check function (with parameters) to call
#
define(`ftestCheck', `dnl
`check = '_qstripTWS(`$1')')

#
# ftestOuput() - print results.
#
# $1: name of result (printed immediately before result)
# $2: result variable to print
#
define(`ftestOutput', `dnl
`ftestPrintTimer( timing_ftestTimer_time );
    ftestPrintCheck( check );
    ftestPrint'_stripTWS(`_ftestOutType_$2')`( $1, $2)'')

dnl switch on output again
divert`'dnl
