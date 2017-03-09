package gov.epa.surrogate;

import java.io.File;

public class FileVerifier {

    public void shouldExist(File file) throws Exception {
        if (!file.exists()) {
            throw new Exception("The file '" + file + "' does not exist");
        }

        if (!file.isFile()) {
            throw new Exception("The file '" + file + "' is not a file");
        }
    }

}
