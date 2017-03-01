@echo off
::  Batch file to start Normalize Surrogates

set MERGE_HOME=D:\CEP\SurrogateTools

set JAVA_EXE="D:\ProgramFiles\Java\jdk1.5.0_05\bin\java"

::  set needed jar files in CLASSPATH

set CLASSPATH=%MERGE_HOME%\SurrogateTools-0.8.jar


@echo on

%JAVA_EXE% -classpath %CLASSPATH% gov.epa.surrogate.merge.Main %1
