/* emacs edit mode for this file is -*- C++ -*- */
/* $Id: ftest_util.cc,v 1.3 1997-09-24 07:28:10 schmidt Exp $ */

//{{{ docu
//
// ftest_util.cc - some utilities for the factory test environment.
//
//}}}

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <GetOpt.h>

// where is the declaration of this function?
extern "C" {
    char * strsignal();
}

// we include timing.h for calculation of HZ only
#define TIMING
#include <timing.h>

#include <factory.h>

#include "ftest_util.h"
#include "ftest_io.h"

//{{{ const int CFSwitchesMax
//{{{ docu
//
// const CFSwitchesMax - maximum number of switches.
//
//}}}
const int CFSwitchesMax = 10;
//}}}

//{{{ struct varSpecT
struct varSpecT
{
    char variable;
    CanonicalForm mipo;
    varSpecT * next;
};
//}}}

//{{{ struct ftestEnvT
//{{{ docu
//
// struct ftestEnvT - factory environment.
//
//}}}
struct ftestEnvT
{
    int seed;
    int characteristic;
    int extDegree;
    char extGen;
    varSpecT * varSpec;
    bool switches[CFSwitchesMax];
};
//}}}

//{{{ external variables
//{{{ docu
//
// - external variables.
//
// ftestCircle: set by ftestGetOpts() from commandline, read by
//   main().  Number of test circles.
// ftestAlarm: set by ftestGetOpts() from acommandline, read by
//   main().
// ftestPrintResultFlag: set by ftestParseOutputType() from
//   commandline, read by output routines. Whether to print result
//   or not.
//
//}}}
int ftestCircle = 1;
int ftestAlarm = 0;
int ftestPrintResultFlag = 1;
//}}}

//{{{ static variables
//{{{ docu
//
// - static variables.
//
// ftestPrint*Flag: set by ftestParseOutputType() from
//   commandline, read by output routines. Things to print/not to
//   print.  Note that we need `ftestPrintResultFlag' to be
//   external.
// ftestExecName, ftestAlgorithmName: set by ftestSetName(), read
//   by output routines.  Name of executable and algorithm.
// ftestUsage: set by ftestSetName(), read by ftestUsage().
//   Algorithm-specific usage information.
// ftestEnv: factory environment specification.  Set by
//   ftestGetEnv(), read by ftestPrintEnv() function.  This
//   variable is quite spurious, but I keep it for future use.
// ftestSwitchNames: symbolic names of switches
//
//
//}}}
static int ftestPrintTimingFlag = 0;
static int ftestPrintCheckFlag = 0;
static int ftestPrintEnvFlag = 0;
static int ftestPrintShortFlag = 1;

const char * ftestExecName = 0;
const char * ftestAlgorithmName = 0;
const char * ftestUsage = 0;

ftestEnvT ftestEnv;

const char * ftestSwitchNames[] =
{
    "RATIONAL",
    "QUOTIENT",
    "SYMMETRIC_FF",
    "BERLEKAMP",
    "FAC_USE_BIG_PRIMES",
    "FAC_QUADRATICLIFT",
    "USE_EZGCD",
    "USE_SPARSEMOD"
};
//}}}

//
// - static functions.
//

//{{{ static const char * ftestSubStr( const char * subString, const char * string )
//{{{ docu
//
// ftestSubStr() - check whether subString is a substring of string.
//
// If so, return index behind subString in string, otherwise
// string.
//
//}}}
static const char *
ftestSubStr( const char * subString, const char * string )
{
    const char * stringStart = string;

    while ( *subString && *subString == *string ) {
	subString++; string++;
    }

    if ( *subString )
	return stringStart;
    else
	return string;
}
//}}}

//{{{ static size_t ftestStrLen( const char * string )
//{{{ docu
//
// ftestStrLen() - return length of string, 0 for zero pointer.
//
//}}}
static size_t
ftestStrLen( const char * string )
{
    return string ? strlen( string ) : 0;
}
//}}}

//{{{ static char * ftestStrCat (char * to, const char * from)
//{{{ docu
//
// ftestStrCat() - concatenate (maybe empty) from and to.
//
//}}}
static char *
ftestStrCat (char * to, const char * from)
{
    if ( from )
	strcat( to, from );
    return to;
}
//}}}

//{{{ static const char * ftestSkipBlancs ( const char * string )
//{{{ docu
//
// ftestSkipBlancs() - skip all leading blancs in string.
//
// Return new position.
//
//}}}
static const char *
ftestSkipBlancs ( const char * string )
{
    while ( *string && isspace( *string ) )
	string++;
    return string;
}
//}}}

//{{{ static char * ftestConcatEnv ( char ** argv, int & optind )
//{{{ docu
//
// ftestConcatEnv() - concatenate all environment information
//   together and return the result.
//
// The new index into the commandline is returned in optind.
//
//}}}
static char *
ftestConcatEnv ( char ** argv, int & optind )
{
    // first get length
    int i = optind;
    int len;
    len = ftestStrLen( getenv( "FTEST_ENV" ) )
	+ ftestStrLen( getenv( "FTEST_SEED" ) )
	+ ftestStrLen( getenv( "FTEST_CHAR" ) )
	+ ftestStrLen( getenv( "FTEST_VARS" ) )
	+ ftestStrLen( getenv( "FTEST_SWITCHES" ) );

    while ( (argv[i] != 0) && (*ftestSkipBlancs( argv[i] ) == '/') ) {
	len += strlen( argv[i] );
	i++;
    }

    char * envString = new char[len+1];

    // now build string
    envString[0] = '\0';
    ftestStrCat( envString, getenv( "FTEST_ENV" ) );
    ftestStrCat( envString, getenv( "FTEST_SEED" ) );
    ftestStrCat( envString, getenv( "FTEST_CHAR" ) );
    ftestStrCat( envString, getenv( "FTEST_VARS" ) );
    ftestStrCat( envString, getenv( "FTEST_SWITCHES" ) );

    while ( optind < i ) {
	strcat( envString, argv[optind] );
	optind++;
    }

    return envString;
}
//}}}

//{{{ static const char * ftestParseSeed ( const char * tokenString )
//{{{ docu
//
// ftestParseSeed() - parse seed specification and set factory's
//   random generator seed.
//
// The results are also stored in global variable `ftestEnv'.
// Returns pointer behind last parsed character in tokenString.
//
//}}}
static const char *
ftestParseSeed ( const char * tokenString )
{
    const char * tokenCursor;

    tokenString++;			// strtol will skip blancs
    int seed = (int)strtol( tokenString, (char **)&tokenCursor, 0 );
    if ( tokenCursor == tokenString )
	ftestError( EnvSyntaxError,
		    "number expected in seed specification `%s'\n",
		    tokenString );

    // set seed
    ftestEnv.seed = seed;
    factoryseed( seed );

    return tokenCursor;
}
//}}}

//{{{ static const char * ftestParseChar ( const char * tokenString )
//{{{ docu
//
// ftestParseChar() - parse characteristic specification and set factory's
//   characteristic.
//
// The results are also stored in global variable `ftestEnv'.
// Returns pointer behind last parsed character in tokenString.
//
//}}}
static const char *
ftestParseChar ( const char * tokenString )
{
    const char * tokenCursor;
    int characteristic = 0;
    int extDegree = 0;
    char extGen = 'Z';

    characteristic = (int)strtol( tokenString, (char **)&tokenCursor, 0 );
    tokenString = ftestSkipBlancs( tokenCursor );

    // look for exponent
    if ( *tokenString == '^' ) {
	tokenString++;
	extDegree = (int)strtol( tokenString, (char **)&tokenCursor, 0 );
	if ( tokenCursor == tokenString )
	    ftestError( EnvSyntaxError,
			"bad exponent in char specification `%s'\n",
			tokenString );
	tokenString = ftestSkipBlancs( tokenCursor );
	// look for generator
	if ( *tokenString == ',' ) {
	    tokenString = ftestSkipBlancs( tokenString+1 );
	    if ( isalpha( *tokenString ) ) {
		extGen = *tokenString;
		tokenString++;
	    } else
		ftestError( EnvSyntaxError,
			    "bad generator in char specification `%s'\n",
			    tokenString );
	}
    }

    // set characteristic
    ftestEnv.characteristic = characteristic;
    if ( extDegree ) {
	ftestEnv.extDegree = extDegree;
	ftestEnv.extGen = extGen;
	setCharacteristic( characteristic, extDegree, extGen );
    } else
	setCharacteristic( characteristic );

    return tokenString;
}
//}}}

//{{{ static const char * ftestParseSwitches ( const char * tokenString )
//{{{ docu
//
// ftestParseSwitches() - parse switch specification and set factory's
//   switches.
//
// The results are also stored in global variable `ftestEnv'.
// Returns pointer behind last parsed character in tokenString.
//
//}}}
static const char *
ftestParseSwitches ( const char * tokenString )
{
    unsigned int switchNumber;
    bool switchSetting;

    while ( *tokenString == '+' || *tokenString == '-') {
	const char * tokenCursor;
	if ( *tokenString == '+' )
	    switchSetting = true;
	else
	    switchSetting = false;
	tokenString = ftestSkipBlancs( tokenString+1 );

	// skip optional leading "SW_"
	tokenString = ftestSubStr( "SW_", tokenString );

	// now search name of switch
	for ( switchNumber = 0;
	      switchNumber < sizeof( ftestSwitchNames ) / sizeof( char* );
	      switchNumber++ )
	    if ( (tokenCursor = ftestSubStr( ftestSwitchNames[switchNumber], tokenString ))
		 != tokenString )
		break;

	if ( tokenCursor == tokenString )
	    ftestError( EnvSyntaxError,
			"unknown switch `%s'\n", tokenString );

	// set switch
	ftestEnv.switches[switchNumber] = switchSetting;
	if ( switchSetting )
	    On( switchNumber );

	tokenString = ftestSkipBlancs( tokenCursor );
    }

    return tokenString;
}
//}}}

//{{{ static const char * ftestParseVars ( const char * tokenString )
//{{{ docu
//
// ftestParseVars() - parse variable specification and set factory's
//   variable.
//
// The results are also stored in global variable `ftestEnv'.
// Returns pointer behind last parsed character in tokenString.
//
//}}}
static const char *
ftestParseVars ( const char * tokenString )
{
    char variable;
    CanonicalForm mipo = 0;
    Variable * dummy;

    // we append at end of list
    static varSpecT * endOfVarSpec = 0;
    varSpecT * varSpec;

    while ( isalpha( *tokenString ) ) {
	variable = *tokenString;
	mipo = 0;

	tokenString = ftestSkipBlancs( tokenString+1 );

	// look for mipo
	if ( *tokenString == '=' ) {
	    const char * tokenCursor;
	    tokenString = ftestSkipBlancs( tokenString+1 );

	    // search for end of mipo
	    tokenCursor = strpbrk( tokenString, ",/" );
	    if ( ! tokenCursor )
		tokenCursor = strchr( tokenString, '\0' );

	    // copy mipo to new string and read it
	    int mipoLen = tokenCursor-tokenString;
	    char * mipoString = new char[mipoLen+1];
	    strncpy( mipoString, tokenString, mipoLen );
	    mipoString[mipoLen] = '\0';
	    mipo = ftestGetCanonicalForm( mipoString );

	    tokenString = ftestSkipBlancs( tokenCursor );
	    delete [] mipoString;
	}

	if ( *tokenString == ',' )
	    // skip optional separator
	    tokenString = ftestSkipBlancs( tokenString+1 );

	// store information in ftestEnv
	varSpec	= new varSpecT;
	varSpec->variable = variable;
	varSpec->mipo = mipo;
	varSpec->next = 0;
	if ( ftestEnv.varSpec )
	    endOfVarSpec->next = varSpec;
	else
	    ftestEnv.varSpec = varSpec;
	endOfVarSpec = varSpec;

	// set variable
	if ( mipo.isZero() )
	    // polynomial variable
	    dummy = new Variable( variable );
	else
	    dummy = new Variable( rootOf( mipo, variable ) );
	// we only need the global information
	delete dummy;
    }

    if ( *tokenString && *tokenString != '/' )
	ftestError( EnvSyntaxError,
		    "extra characters in variable specification `%s'\n",
		    tokenString );

    return tokenString;
}
//}}}

//{{{ static void ftestParseEnv ( const char * tokenString )
//{{{ docu
//
// ftestParseEnv() - parse environment specification and set factory's
//   environment.
//
// The results are also stored in global variable `ftestEnv'.
// Returns pointer behind last parsed character in tokenString.
//
//}}}
static void
ftestParseEnv ( const char * tokenString )
{
    tokenString = ftestSkipBlancs( tokenString );
    while ( *tokenString == '/' ) {

	tokenString = ftestSkipBlancs( tokenString+1 );

	if ( *tokenString == '@' ) {
	    // seed specification
	    tokenString = ftestParseSeed( tokenString );
	} else if ( isdigit( *tokenString ) ) {
	    // specification of characteristics
	    tokenString = ftestParseChar( tokenString );
	} else if ( *tokenString == '+' || *tokenString == '-' ) {
	    // specification of switches
	    tokenString = ftestParseSwitches( tokenString );
	} else if ( isalpha( *tokenString ) ) {
	    // specification of variables
	    tokenString = ftestParseVars( tokenString );
	}
	tokenString = ftestSkipBlancs( tokenString );
    }
    
    // check if we reached end
    if ( *tokenString )
	ftestError( EnvSyntaxError,
		    "extra characters in environment specification `%s'\n",
		    tokenString );
}
//}}}

//{{{ static void ftestParseOutputType ( const char * outputType )
//{{{ docu
//
// ftestParseOutputType() - parse output type and set ftestPrint*Flags.
//
//}}}
static void
ftestParseOutputType ( const char * outputType )
{
    ftestPrintResultFlag = 0;
    ftestPrintShortFlag = 0;
    while ( *outputType ) {
	switch ( *outputType ) {
	case 'r': ftestPrintResultFlag = 1; break;
	case 't': ftestPrintTimingFlag = 1; break;
	case 'c': ftestPrintCheckFlag = 1; break;
	case 'e': ftestPrintEnvFlag = 1; break;
	case 's': ftestPrintShortFlag = 1; break;
	case 'a':
	    ftestPrintResultFlag = 1;
	    ftestPrintCheckFlag = 1;
	    ftestPrintTimingFlag = 1; break;
	default: ftestError( CommandlineError, "unknown output type specifier `%c'\n", *outputType );
	}
	outputType++;
    }
}
//}}}

//{{{ static void ftestSignalHandler ( int signal ), static void ftestAlarmlHandler ( int )
//{{{ docu
//
// ftestSignalHandler(), ftestAlarmHandler() - signal handlers.
//
// Simply calls `ftestError()'.
//
//}}}
static void
ftestSignalHandler ( int signal )
{
    ftestError( (ftestErrorT)((int)SignalError + signal ),
		"received signal `%s'\n", strsignal( signal ) );
}
static void
ftestAlarmHandler ( int )
{
    ftestError( TimeoutError, "timeout after %d secs\n", ftestAlarm );
}
//}}}

//
// - external functions.
//

//{{{ void ftestError ( const ftestErrorT errno, const char * format ... )
//{{{ docu
//
// ftestError() - main error handler.
//
// Prints error message consisting of formatString and following
// arguments, some additional information, and exits with errno.
//
//}}}
void
ftestError ( const ftestErrorT errno, const char * format ... )
{
    // print error message
    if ( format ) {
	fprintf( stderr, "%s: ", ftestExecName );
	va_list ap;
	va_start( ap, format );
	vfprintf( stderr, format, ap );
	va_end( ap );
    }

    switch ( errno ) {
    case CommandlineError:
    case EnvSyntaxError: 
	// ftestUsage();
	break;
    case TimeoutError:
	if ( ftestPrintTimingFlag )
	    ftestPrint( "time  : > %.2f\n", "> %.2f\n", (float)ftestAlarm );
	if ( ftestPrintCheckFlag )
	    ftestPrint( "check : TmeOut\n", "TmeOut\n" );
	if ( ftestPrintResultFlag )
	    ftestPrint( "result: <timeout> =\n0\n", "0\n" );
	break;
    default:
	if ( ftestPrintTimingFlag )
	    ftestPrint( "time  : -0.00\n", "-0.00\n" );
	if ( ftestPrintCheckFlag )
	    ftestPrint( "check : Sig.%0.2d\n", "Sig.%0.2d\n", (int)errno-(int)SignalError );
	if ( ftestPrintResultFlag )
	    ftestPrint( "result: <signal %s> =\n0\n", "0\n",
			strsignal( (int)errno-(int)SignalError ) );
    }
    exit( errno );
}
//}}}

//{{{ void ftestPrint ( const char * longFormat, const char * shortFormat ... )
//{{{ docu
//
// ftestPrint() - print according to `ftestPrintShortFlag'.
//
// If variable ftestPrintShortFlag is set, use shortFormat as
// format, otherwise longFormat.
//
//}}}
void
ftestPrint ( const char * longFormat, const char * shortFormat ... )
{
    va_list ap;
    va_start( ap, shortFormat );

    if ( ! ftestPrintShortFlag && longFormat )
	vprintf( longFormat, ap );
    else if ( ftestPrintShortFlag && shortFormat )
	vprintf( shortFormat, ap );

    va_end( ap );
}
//}}}

//{{{ void ftestSignalCatch ()
//{{{ docu
//
// ftestSignalCatch() - set signal handlers.
//
//}}}
void
ftestSignalCatch ()
{
#ifdef SIGHUP
    signal( SIGHUP, ftestSignalHandler );
#endif
#ifdef SIGINT
    signal( SIGINT, ftestSignalHandler );
#endif
#ifdef SIGQUIT
    signal( SIGQUIT, ftestSignalHandler );
#endif
#ifdef SIGILL
    signal( SIGILL, ftestSignalHandler );
#endif
#ifdef SIGIOT
    signal( SIGIOT, ftestSignalHandler );
#endif
#ifdef SIGFPE
    signal( SIGFPE, ftestSignalHandler );
#endif
#ifdef SIGBUS
    signal( SIGBUS, ftestSignalHandler );
#endif
#ifdef SIGSEGV
    signal( SIGSEGV, ftestSignalHandler );
#endif

    // alarm handler
    signal( SIGALRM, ftestAlarmHandler );
}
//}}}

//{{{ void ftestSetName ( const char * execName, const char * algorithmName, const char * usage )
//{{{ docu
//
// ftestSetName() - set name of executable and algorithm.
//
//}}}
void
ftestSetName ( const char * execName, const char * algorithmName, const char * usage )
{
    ftestExecName = execName;
    ftestAlgorithmName = algorithmName;
    ftestUsage = usage;
}
//}}}

//{{{ void ftestGetOpts ( const int argc, char ** argv, int & optind )
//{{{ docu
//
// ftestGetOpts() - read options from commandline.
//
// Returns new index into commandline in `optind'.
//
//}}}
void
ftestGetOpts ( const int argc, char ** argv, int & optind )
{
    // parse command line
    GetOpt getopt( argc, argv, "a:o:c:" );
    int optionChar;
    char * outputType = getenv( "FTEST_OUTPUT" );

    // parse options
    while ( (optionChar = getopt()) != EOF ) {
	switch ( optionChar ) {
	case 'a': ftestAlarm = (int)strtol( getopt.optarg, 0, 0 ); break;
	case 'c': ftestCircle = (int)strtol( getopt.optarg, 0, 0 ); break;
	case 'o': outputType = getopt.optarg; break;
	default: ftestError( CommandlineError, 0 );
	}
    }
    optind = getopt.optind;

    if ( outputType )
	ftestParseOutputType( outputType );
}
//}}}

//{{{ void ftestGetEnv ( const int, char ** argv, int & optind )
//{{{ docu
//
// ftestGetEnv() - read factory environment specs, set environment.
//
// The new index into the commandline is returned in optind.
// The results are stored in global variable `ftestEnv'.
//
//}}}
void
ftestGetEnv ( const int, char ** argv, int & optind )
{
    // initialize environment
    ftestEnv.seed = 0;
    ftestEnv.characteristic = 0;
    ftestEnv.extDegree = 0;
    ftestEnv.extGen = 'Z';
    ftestEnv.varSpec = 0;
    for ( int i = 0; i < CFSwitchesMax; i++ )
	ftestEnv.switches[i] = false;

    char * envString = ftestConcatEnv( argv, optind );
    ftestParseEnv( envString );
    delete [] envString;
}
//}}}

//{{{ void ftestPrintTimer ( const long timer )
//{{{ docu
//
// ftestPrintTimer() - print value of timer.
//
//}}}
void
ftestPrintTimer ( const long timer )
{
    if ( ftestPrintTimingFlag )
	ftestPrint( "time  : %.2f\n", "%.2f\n", (float)timer / HZ );
}
//}}}

//{{{ void ftestPrintCheck ( const ftestSatusT check )
//{{{ docu
//
// ftestPrintCheck() - print status of checks.
//
//}}}
void
ftestPrintCheck ( const ftestStatusT check )
{
    if ( ftestPrintCheckFlag ) {
	char * checkStr = 0;

	switch (check) {
	case Passed: checkStr = "Passed"; break;
	case Failed: checkStr = "Failed"; break;
	case UndefinedResult: checkStr = "Undef."; break;
	default: checkStr = "Error!";
	}

	ftestPrint( "check : %s\n", "%s\n", checkStr );
    }
}
//}}}

//{{{ void ftestPrintEnv ()
//{{{ docu
//
// ftestPrintEnv() - print current factory environment.
//
// The environment ist read from `ftestEnv'.
//
//}}}
void
ftestPrintEnv ()
{
    if ( ftestPrintEnvFlag ) {
	// characteristic
	if ( ftestEnv.extDegree )
	    ftestPrint( "char  : %d^%d (%c)\n",
			"%d^%d (%c)\n",
			ftestEnv.characteristic, ftestEnv.extDegree, ftestEnv.extGen );
	else
	    ftestPrint( "char  : %d\n", "%d\n", ftestEnv.characteristic );

	// switches
	ftestPrint( "switch: ", (char *)0 );
	for ( int i = 0; i < CFSwitchesMax; i++ )
	    if ( ftestEnv.switches[i] )
		printf( "%s ", ftestSwitchNames[i] );
	printf( "\n" );

	// variables
	varSpecT * varSpec = ftestEnv.varSpec;
	ftestPrint( "vars  : ", (char *)0 );
	while ( varSpec ) {
	    if ( varSpec->mipo.isZero() )
		printf( "%c(%d) ",
			varSpec->variable,
			Variable( varSpec->variable ).level() );
	    else {
		printf( "%c(%d)",
			varSpec->variable,
			Variable( varSpec->variable ).level() );
		cout << "(" << varSpec->mipo << ") ";
	    }
	    varSpec = varSpec->next;
	}
	printf( "\n" );

	// factory version and other information
	ftestPrint( "circle: %d\n", "%d\n", ftestCircle );
	ftestPrint( "seed  : %d\n", "%d\n", ftestEnv.seed );

	const char * version;
	version = strchr( factoryVersion, '=' )+2;
	ftestPrint( "vers  : %s\n", "%s\n", version );
    }
}
//}}}

//
// - garbage.
//

//{{{ static void ftestSetSeed ()
//{{{ docu
//
// ftestSetSeed() - set random generator seed from `ftestEnv'.
//
// This function is quite spurious, but I keep it for future
// use.
//
//}}}
static void
ftestSetSeed ()
{
    factoryseed( ftestEnv.seed );
}
//}}}

//{{{ static void ftestSetChar ( bool reset = false )
//{{{ docu
//
// ftestSetChar() - set characteristic from `ftestEnv'.
//
// If reset is true, reset characteristic to default.
//
// This function is quite spurious, but I keep it for future
// use.
//
//}}}
static void
ftestSetChar ( bool reset = false )
{
    if ( reset )
	setCharacteristic( 0 );
    else
	if ( ftestEnv.extDegree )
	    setCharacteristic( ftestEnv.characteristic,
			       ftestEnv.extDegree,
			       ftestEnv.extGen );
	else
	    setCharacteristic( ftestEnv.characteristic );
}
//}}}

//{{{ static void ftestSetSwitches ( bool reset = false )
//{{{ docu
//
// ftestSetSwitches() - set switches from `ftestEnv'.
//
// If reset is true, reset switches to default.
//
// This function is quite spurious, but I keep it for future
// use.
//
//}}}
static void
ftestSetSwitches ( bool reset = false )
{
    int i;

    if ( reset )
	for ( i = 0; i < CFSwitchesMax; i ++ )
	    Off( i );
    else
	for ( i = 0; i < CFSwitchesMax; i ++ )
	    if ( ftestEnv.switches[i] )
		On( i );
}
//}}}

//{{{ static void ftestSetVars ()
//{{{ docu
//
// ftestSetVars() - set variables from `ftestEnv'.
//
// This function is quite spurious, but I keep it for future
// use.
//
//}}}
static void
ftestSetVars ()
{
    varSpecT * varSpec = ftestEnv.varSpec;
    Variable * dummy;

    while ( varSpec ) {
	if ( varSpec->mipo.isZero() )
	    // polynomial variable
	    dummy = new Variable( varSpec->variable );
	else
	    dummy = new Variable( rootOf( varSpec->mipo, varSpec->variable ) );

	varSpec = varSpec->next;
    }
}
//}}}

//{{{ void ftestSetEnv ()
//{{{ docu
//
// ftestSetEnv() - set environment from `ftestEnv'.
//
// This function is quite spurious, but I keep it for future
// use.
//
//}}}
void
ftestSetEnv ()
{
    ftestSetSeed();
    ftestSetSwitches();
    ftestSetChar();
    ftestSetVars();
}
//}}}
