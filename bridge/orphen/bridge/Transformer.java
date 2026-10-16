package orphen.bridge;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import org.objectweb.asm.*;

public final class Transformer {
    public static byte[] bridgeBytes() throws IOException {
        try (InputStream in = Transformer.class.getResourceAsStream("TickBridge.class")) {
            if (in == null) throw new IOException("TickBridge.class missing");
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] buffer = new byte[4096];
            for (int n; (n = in.read(buffer)) != -1;) out.write(buffer, 0, n);
            return out.toByteArray();
        }
    }

    public static byte[] instrument(byte[] original) {
        final int[] matches = {0};
        ClassReader reader = new ClassReader(original);
        ClassWriter writer = new ClassWriter(reader, 0);
        reader.accept(new ClassVisitor(Opcodes.ASM9, writer) {
            @Override public MethodVisitor visitMethod(int access, String name, String desc,
                                                       String signature, String[] exceptions) {
                MethodVisitor delegate = super.visitMethod(access, name, desc, signature, exceptions);
                if (!name.equals("runTick") || !desc.equals("()V")
                        || (access & (Opcodes.ACC_NATIVE | Opcodes.ACC_ABSTRACT | Opcodes.ACC_STATIC)) != 0)
                    return delegate;
                matches[0]++;
                return new MethodVisitor(Opcodes.ASM9, delegate) {
                    @Override public void visitCode() {
                        super.visitCode();
                        // No operands: stack height and existing frame types are unchanged.
                        super.visitMethodInsn(Opcodes.INVOKESTATIC,
                                "orphen/bridge/TickBridge", "tick", "()V", false);
                    }
                };
            }
        }, 0);
        if (matches[0] != 1) throw new IllegalStateException("Expected exactly one runTick()V");
        return writer.toByteArray();
    }
}
