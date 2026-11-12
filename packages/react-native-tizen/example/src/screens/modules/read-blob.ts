/** `FileReader.readAsText` or `readAsDataURL`, as a promise. */
export function readBlob(blob: Blob, as: 'text' | 'dataURL'): Promise<string> {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onload = () => {
      if (typeof reader.result === 'string') resolve(reader.result);
      else reject(new Error('FileReader returned no string'));
    };
    reader.onerror = () => reject(new Error(reader.error?.message ?? 'FileReader failed'));
    if (as === 'text') reader.readAsText(blob);
    else reader.readAsDataURL(blob);
  });
}
