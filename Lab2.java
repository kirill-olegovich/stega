import java.io.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.security.MessageDigest;
import java.util.*;

public class Lab2 {

    static class FileHeader {
        int id;
        long fSize;
        int rez1, rez2;
        long bmOffset;
    }

    static class V3Header {
        long hSize;
        long width;
        long height;
        int planes, bitPerPixel;
        long compression;
        long sizeImage;
        long hRes;
        long vRes;
        long clrUsed;
        long clrImp;
    }

    static class ColorInfo {
        int blue, green, red, temp;
    }

    static int stableSeed(String s) {
        return Arrays.hashCode(s.getBytes());
    }

    public static void main(String[] args) throws Exception {
        Scanner sc = new Scanner(System.in);

        int mode = 0;
        System.out.print("Mode -> ");
        mode = sc.nextInt();

        System.out.print("File -> ");
        String pOrig = sc.next();

        System.out.print("Logo -> ");
        String pLogo = sc.next();

        try (RandomAccessFile fileIn = new RandomAccessFile(pOrig, "r");
             RandomAccessFile fileLogo = new RandomAccessFile(pLogo, "r")) {
            if (mode == 1) {
                encodeWithLSB(fileIn, fileLogo);
            } else if (mode == 2) {
                encodeWithLocalVariance(fileIn, fileLogo);
            } else if (mode == 3) {
                decodeWithLSB(fileIn, fileLogo);
            } else {
                decodeWithLocalVariance(fileIn, fileLogo);
            }
        }
    }

    static private void encodeWithLSB(RandomAccessFile fileIn, RandomAccessFile fileLogo) throws Exception {
        FileHeader fileHeaderIn = readFileHeader(fileIn);
        V3Header v3In = readV3Header(fileIn);
        ColorInfo[] colorInfoIn = readColorTable(fileIn, 256);
        fileIn.seek(fileHeaderIn.bmOffset);

        FileHeader fileHeaderLogo = readFileHeader(fileLogo);
        V3Header v3Logo = readV3Header(fileLogo);
        ColorInfo[] colorInfoLogo = readColorTable(fileLogo, 256);
        fileLogo.seek(fileHeaderLogo.bmOffset);

        try (RandomAccessFile out = new RandomAccessFile("encoded_with_LSB.bmp", "rw")) {
            out.setLength(0);
            writeFileHeader(out, fileHeaderIn);
            writeV3Header(out, v3In);
            writeColorTable(out, colorInfoIn);

            int[] indx = new int[(int)v3In.width];
            for (int i = 0; i < indx.length; i++) indx[i] = i;
            Random rng = new Random(stableSeed("KEY"));
            for (int i = indx.length - 1; i > 0; i--) {
                int j = rng.nextInt(i + 1);
                int t = indx[i];
                indx[i] = indx[j];
                indx[j] = t;
            }

            int bytesPerPixelIn = v3In.bitPerPixel / 8;
            int rowSizeIn = ((int) (v3In.width * bytesPerPixelIn + 3) / 4) * 4;
            int bytesPerPixelLogo = v3Logo.bitPerPixel / 8;
            int rowSizeLogo = ((int) (v3Logo.width * bytesPerPixelLogo + 3) / 4) * 4;

            byte[] rowIn = new byte[rowSizeIn];
            byte[] rowLogo = new byte[rowSizeLogo];
            int point = 0;
            int count = 0;
            int ccount = 0;
            int readLogo = fileLogo.read(rowLogo);

            for (int y = 0; y < v3In.height; y++) {
                int readIn = fileIn.read(rowIn);
                if (readIn <= 0) break;

                for (int x = 0; x < v3In.width; x++) {
                    if (readLogo <= 0) break;

                    rowIn[indx[x]] = (byte) ((rowIn[indx[x]] & 0xFC) | (rowLogo[point] & 3));
                    rowLogo[point] = (byte) ((rowLogo[point] & 0xFF) >>> 2);
                    count += 2;
                    ccount += 2;

                    if (count == 8) {
                        point++;
                        count = 0;
                    }

                    if (point == v3Logo.width) {
                        readLogo = fileLogo.read(rowLogo);
                        if (readLogo <= 0) break;
                        point = 0;
                    }
                }

                out.write(rowIn, 0, rowSizeIn);
            }

            System.out.println("Encoded " + ccount + " symbols");
        }
    }

    static private void decodeWithLSB(RandomAccessFile fileIn, RandomAccessFile fileLogo) throws Exception {
        FileHeader fileHeaderIn = readFileHeader(fileIn);
        V3Header v3In = readV3Header(fileIn);
        ColorInfo[] colorInfoIn = readColorTable(fileIn, 256);
        fileIn.seek(fileHeaderIn.bmOffset);

        FileHeader fileHeaderLogo = readFileHeader(fileLogo);
        V3Header v3Logo = readV3Header(fileLogo);
        ColorInfo[] colorInfoLogo = readColorTable(fileLogo, 256);
        fileLogo.seek(fileHeaderLogo.bmOffset);

        int[] indx = new int[(int)v3In.width];
        for (int i = 0; i < indx.length; i++) indx[i] = i;
        Random rng = new Random(stableSeed("KEY"));
        for (int i = indx.length - 1; i > 0; i--) {
            int j = rng.nextInt(i + 1);
            int t = indx[i];
            indx[i] = indx[j];
            indx[j] = t;
        }

        int bytesPerPixelIn = v3In.bitPerPixel / 8;
        int rowSizeIn = ((int) (v3In.width * bytesPerPixelIn + 3) / 4) * 4;
        int bytesPerPixelLogo = v3Logo.bitPerPixel / 8;
        int rowSizeLogo = ((int) (v3Logo.width * bytesPerPixelLogo + 3) / 4) * 4;

        byte[] rowIn = new byte[rowSizeIn];
        byte[] rowLogo = new byte[rowSizeLogo];
        int point = 0;
        int count = 0;
        int ccount = 0;
        int temp = 0;
        int errors = 0;
        int readLogo = fileLogo.read(rowLogo);

        for (int y = 0; y < v3In.height; y++) {
            int readIn = fileIn.read(rowIn);
            if (readIn <= 0) break;

            for (int x = 0; x < v3In.width; x++) {
                if (readLogo <= 0) break;

                temp |= ((rowIn[indx[x]] & 3) << count);
                count += 2;
                ccount += 2;

                if (count == 8) {
                    if (temp != (rowLogo[point] & 0xFF)) errors++;
                    count = 0;
                    point++;
                    temp = 0;
                }

                if (point == v3Logo.width) {
                    readLogo = fileLogo.read(rowLogo);
                    if (readLogo <= 0) break;
                    point = 0;
                }
            }
        }

        System.out.println("Check logo result = " + errors + " errors");
        System.out.println("Decoded = " + ccount);
    }

    static private void encodeWithLocalVariance(RandomAccessFile fileIn, RandomAccessFile fileLogo) throws Exception {
        FileHeader fileHeaderIn = readFileHeader(fileIn);
        V3Header v3In = readV3Header(fileIn);
        ColorInfo[] colorInfoIn = readColorTable(fileIn, 256);
        fileIn.seek(fileHeaderIn.bmOffset);

        FileHeader fileHeaderLogo = readFileHeader(fileLogo);
        V3Header v3Logo = readV3Header(fileLogo);
        ColorInfo[] colorInfoLogo = readColorTable(fileLogo, 256);
        fileLogo.seek(fileHeaderLogo.bmOffset);

        List<byte[]> mas = new ArrayList<>();
        int bytesPerPixelIn = v3In.bitPerPixel / 8;
        int rowSizeIn = ((int) (v3In.width * bytesPerPixelIn + 3) / 4) * 4;
        // int bpr = rowBytes((int)v3In.width, v3In.bitPerPixel);
        byte[] rowBuf = new byte[rowSizeIn];
        while (true) {
            int rs = fileIn.read(rowBuf);
            if (rs <= 0) break;
            byte[] row = new byte[rs];
            System.arraycopy(rowBuf, 0, row, 0, rs);
            mas.add(row);
        }

        int totalPixels = (int)(v3In.width * v3In.height);
        double[] variance = new double[totalPixels];
        final int BLOCK_SIZE = 8;

        for (int r = 0; r < v3In.height; r++) {
            for (int c = 0; c < v3In.width; c++) {
                int idx = (int)(r * v3In.width + c);
                
                int startX = Math.max(0, c - BLOCK_SIZE/2);
                int endX = Math.min((int)v3In.width, c + BLOCK_SIZE/2 + 1);
                int startY = Math.max(0, r - BLOCK_SIZE/2);
                int endY = Math.min((int)v3In.height, r + BLOCK_SIZE/2 + 1);

                int count = 0;
                double sum = 0, sumSq = 0;
                
                for (int by = startY; by < endY; by++) {
                    for (int bx = startX; bx < endX; bx++) {
                        int val = mas.get(by)[bx] & 0xFC;
                        sum += val;
                        sumSq += val * val;
                        count++;
                    }
                }

                double mean = sum / count;
                variance[idx] = (sumSq / count) - (mean * mean);
            }
        }

        Integer[] indx = new Integer[totalPixels];
        for (int i = 0; i < totalPixels; i++) indx[i] = i;
        Arrays.sort(indx, (a, b) -> {
            if (variance[a] != variance[b]) return Double.compare(variance[b], variance[a]);
            return Integer.compare(a, b);
        });

        int bytesPerPixelLogo = v3Logo.bitPerPixel / 8;
        int rowSizeLogo = ((int) (v3Logo.width * bytesPerPixelLogo + 3) / 4) * 4;
        // int logoBpr = rowBytes((int)v3Logo.width, v3Logo.bitPerPixel);
        byte[] buf = new byte[rowSizeLogo];
        int point = 0, count = 0, ccount = 0;

        while (true) {
            int rs = fileLogo.read(buf);
            if (rs <= 0) break;

            for (int i = 0; i < v3Logo.width; ) {
                if (point >= totalPixels) break;

                int pixelIdx = indx[point];
                int x = pixelIdx % (int)v3In.width;
                int y = pixelIdx / (int)v3In.width;

                mas.get(y)[x] = (byte)((mas.get(y)[x] & 0xFC) | (buf[i] & 3));
                buf[i] = (byte)((buf[i] & 0xFF) >>> 2);

                count += 2;
                ccount += 2;
                point++;
                
                if (count == 8) {
                    count = 0;
                    i++;
                }
            }
            if (point >= totalPixels) break;
        }

        System.out.println("Coded = " + ccount);

        try (RandomAccessFile out = new RandomAccessFile("encoded_with_variance.bmp", "rw")) {
            out.setLength(0);
            writeFileHeader(out, fileHeaderIn);
            writeV3Header(out, v3In);
            writeColorTable(out, colorInfoIn);
            out.seek(fileHeaderIn.bmOffset);

            for (byte[] row : mas) {
                out.write(row);
            }
        }
    }

    static private void decodeWithLocalVariance(RandomAccessFile fileIn, RandomAccessFile fileLogo) throws Exception {
        FileHeader fileHeaderIn = readFileHeader(fileIn);
        V3Header v3In = readV3Header(fileIn);
        ColorInfo[] colorInfoIn = readColorTable(fileIn, 256);
        fileIn.seek(fileHeaderIn.bmOffset);

        FileHeader fileHeaderLogo = readFileHeader(fileLogo);
        V3Header v3Logo = readV3Header(fileLogo);
        ColorInfo[] colorInfoLogo = readColorTable(fileLogo, 256);
        fileLogo.seek(fileHeaderLogo.bmOffset);

        List<byte[]> mas = new ArrayList<>();
        int bytesPerPixelIn = v3In.bitPerPixel / 8;
        int rowSizeIn = ((int) (v3In.width * bytesPerPixelIn + 3) / 4) * 4;
        // int bpr = rowBytes((int)v3In.width, v3In.bitPerPixel);
        byte[] rowBuf = new byte[rowSizeIn];
        while (true) {
            int rs = fileIn.read(rowBuf);
            if (rs <= 0) break;
            byte[] row = new byte[rs];
            System.arraycopy(rowBuf, 0, row, 0, rs);
            mas.add(row);
        }

        int totalPixels = (int)(v3In.width * v3In.height);
        double[] variance = new double[totalPixels];
        final int BLOCK_SIZE = 8;

        for (int r = 0; r < v3In.height; r++) {
            for (int c = 0; c < v3In.width; c++) {
                int idx = (int)(r * v3In.width + c);
                int startX = Math.max(0, c - BLOCK_SIZE/2);
                int endX = Math.min((int)v3In.width, c + BLOCK_SIZE/2 + 1);
                int startY = Math.max(0, r - BLOCK_SIZE/2);
                int endY = Math.min((int)v3In.height, r + BLOCK_SIZE/2 + 1);

                int count = 0;
                double sum = 0, sumSq = 0;

                for (int by = startY; by < endY; by++) {
                    for (int bx = startX; bx < endX; bx++) {
                        int val = mas.get(by)[bx] & 0xFC; 
                        sum += val;
                        sumSq += val * val;
                        count++;
                    }
                }

                double mean = sum / count;
                variance[idx] = (sumSq / count) - (mean * mean);
            }
        }

        Integer[] indx = new Integer[totalPixels];
        for (int i = 0; i < totalPixels; i++) indx[i] = i;
        Arrays.sort(indx, (a, b) -> {
            if (variance[a] != variance[b]) return Double.compare(variance[b], variance[a]); // Descending
            return Integer.compare(a, b);
        });

        int bytesPerPixelLogo = v3Logo.bitPerPixel / 8;
        int rowSizeLogo = ((int) (v3Logo.width * bytesPerPixelLogo + 3) / 4) * 4;
        // int logoBpr = rowBytes((int)v3Logo.width, v3Logo.bitPerPixel);
        byte[] buf = new byte[rowSizeLogo];
        int point = 0, count = 0, ccount = 0, errors = 0;
        int temp = 0;

        while (true) {
            int rs = fileLogo.read(buf);
            if (rs <= 0) break;

            for (int i = 0; i < v3Logo.width; ) {
                if (point >= totalPixels) break;
                int pixelIdx = indx[point];
                int x = pixelIdx % (int)v3In.width;
                int y = pixelIdx / (int)v3In.width;

                temp |= ((mas.get(y)[x] & 3) << count);
                count += 2;
                ccount += 2;
                point++;

                if (count == 8) {
                    if ((temp & 0xFF) != (buf[i] & 0xFF)) errors++;
                    count = 0;
                    temp = 0;
                    i++;
                }
            }
            if (point >= totalPixels) break;
        }

        System.out.println("Check logo result = " + errors + " errors");
        System.out.println("Decoded = " + ccount);
    }

    static FileHeader readFileHeader(RandomAccessFile in) throws IOException {
        FileHeader h = new FileHeader();
        h.id = Short.toUnsignedInt(Short.reverseBytes(in.readShort()));
        h.fSize = Integer.toUnsignedLong(Integer.reverseBytes(in.readInt()));
        h.rez1 = Short.toUnsignedInt(Short.reverseBytes(in.readShort()));
        h.rez2 = Short.toUnsignedInt(Short.reverseBytes(in.readShort()));
        h.bmOffset = Integer.toUnsignedLong(Integer.reverseBytes(in.readInt()));
        return h;
    }

    static V3Header readV3Header(RandomAccessFile in) throws IOException {
        V3Header h = new V3Header();
        h.hSize = Integer.toUnsignedLong(Integer.reverseBytes(in.readInt()));
        h.width = Integer.toUnsignedLong(Integer.reverseBytes(in.readInt()));
        h.height = Integer.toUnsignedLong(Integer.reverseBytes(in.readInt()));
        h.planes = Short.toUnsignedInt(Short.reverseBytes(in.readShort()));
        h.bitPerPixel = Short.toUnsignedInt(Short.reverseBytes(in.readShort()));
        h.compression = Integer.toUnsignedLong(Integer.reverseBytes(in.readInt()));
        h.sizeImage = Integer.toUnsignedLong(Integer.reverseBytes(in.readInt()));
        h.hRes = Integer.toUnsignedLong(Integer.reverseBytes(in.readInt()));
        h.vRes = Integer.toUnsignedLong(Integer.reverseBytes(in.readInt()));
        h.clrUsed = Integer.toUnsignedLong(Integer.reverseBytes(in.readInt()));
        h.clrImp = Integer.toUnsignedLong(Integer.reverseBytes(in.readInt()));
        return h;
    }

    static ColorInfo[] readColorTable(RandomAccessFile in, int n) throws IOException {
        ColorInfo[] table = new ColorInfo[n];
        for (int i = 0; i < n; i++) {
            ColorInfo c = new ColorInfo();
            c.blue = in.readUnsignedByte();
            c.green = in.readUnsignedByte();
            c.red = in.readUnsignedByte();
            c.temp = in.readUnsignedByte();
            table[i] = c;
        }
        return table;
    }

    static void writeFileHeader(RandomAccessFile out, FileHeader h) throws IOException {
        out.writeShort(Short.reverseBytes((short) h.id));
        out.writeInt(Integer.reverseBytes((int) h.fSize));
        out.writeShort(Short.reverseBytes((short) h.rez1));
        out.writeShort(Short.reverseBytes((short) h.rez2));
        out.writeInt(Integer.reverseBytes((int) h.bmOffset));
    }

    static void writeV3Header(RandomAccessFile out, V3Header h) throws IOException {
        out.writeInt(Integer.reverseBytes((int) h.hSize));
        out.writeInt(Integer.reverseBytes((int) h.width));
        out.writeInt(Integer.reverseBytes((int) h.height));
        out.writeShort(Short.reverseBytes((short) h.planes));
        out.writeShort(Short.reverseBytes((short) h.bitPerPixel));
        out.writeInt(Integer.reverseBytes((int) h.compression));
        out.writeInt(Integer.reverseBytes((int) h.sizeImage));
        out.writeInt(Integer.reverseBytes((int) h.hRes));
        out.writeInt(Integer.reverseBytes((int) h.vRes));
        out.writeInt(Integer.reverseBytes((int) h.clrUsed));
        out.writeInt(Integer.reverseBytes((int) h.clrImp));
    }

    static void writeColorTable(RandomAccessFile out, ColorInfo[] table) throws IOException {
        for (ColorInfo c : table) {
            out.writeByte(c.blue);
            out.writeByte(c.green);
            out.writeByte(c.red);
            out.writeByte(c.temp);
        }
    }
}