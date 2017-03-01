@echo off
::  Batch file to start Normalize Surrogates

set GAPFILL_HOME=D:\CEP\SurrogateTools

set JAVA_EXE="D:\ProgramFiles\Java\jdk1.5.0_05\bin\java"

::  set needed jar files in CLASSPATH

set CLASSPATH=%GAPFILL_HOME%\SurrogateTools-0.8.jar


@echo on

%JAVA_EXE% -classpath %CLASSPATH% gov.epa.surrogate.gapfill.Main %1
