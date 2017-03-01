package gov.epa.surrogate.legacy;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

/*--------class to handle Runtime class errors   for executive external exe files--------*/
class StreamGobbler extends Thread {
    InputStream is;

    String type;

    StringBuffer os;

    String LS = System.getProperty("line.separator");

    StreamGobbler(InputStream is, String type) {
        this(is, type, null);
    }

    StreamGobbler(InputStream is, String type, StringBuffer redirect) {
        this.is = is;
        this.type = type;
        this.os = redirect;
    }

    public void run() {
        try {

            InputStreamReader isr = new InputStreamReader(is);
            BufferedReader br = new BufferedReader(isr);
            String line = null;
            while ((line = br.readLine()) != null) {
                os.append(type + ">" + line + LS);
                // System.out.println(type + ">" + line);
            }
        } catch (IOException ioe) {
            os.append("ERROR>" + ioe.getMessage() + LS);
            ioe.printStackTrace();
        }
    }
}