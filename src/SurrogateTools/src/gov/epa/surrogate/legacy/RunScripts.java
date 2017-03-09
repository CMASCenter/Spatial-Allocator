package gov.epa.surrogate.legacy;

/*----------------class to run a script file----------------------*/
class RunScripts {
    protected String[] cmd, env;

    protected String exeFile;

    StringBuffer mBuff = new StringBuffer(); // String to store message from

    // the run

    String LS = System.getProperty("line.separator");

    // construct the class and check the file

    RunScripts(String exe, String[] command, String[] environments) {

        exeFile = exe;
        cmd = new String[command.length];
        env = new String[environments.length];
        System.arraycopy(command, 0, cmd, 0, command.length);
        System.arraycopy(environments, 0, env, 0, environments.length);
    }

    public String run() {
        int exitVal = -1; // run status: 0 --> normal , other --> failed

        try {
            Runtime rt = Runtime.getRuntime();
            Process proc = rt.exec(cmd, env);

            // any error message
            StreamGobbler errorGobbler = new StreamGobbler(proc.getErrorStream(), exeFile + "_ERROR", mBuff);

            // any output?
            StreamGobbler outputGobbler = new StreamGobbler(proc.getInputStream(), exeFile + "_OUTPUT", mBuff);

            // kick them off
            errorGobbler.start();
            outputGobbler.start();

            // any error???
            exitVal = proc.waitFor();

        } catch (Throwable t) {
            mBuff.append(exeFile + "_ERROR>" + t.getMessage() + LS);
        }

        if (exitVal == 0) {
            // System.out.println(exitVal+ " "+cmd[2]+" run succeed");
            mBuff.append("SUCCESS IN RUNNING THE EXECUTABLE: " + exeFile + LS);
        } else {
            // System.out.println(exitVal +" "+cmd[2]+ " run failed");
            mBuff.append("ERROR IN RUNNING THE EXECUTABLE: " + exeFile + LS);
        }

        System.runFinalization();
        System.gc();
        return mBuff.toString();
    }
}
