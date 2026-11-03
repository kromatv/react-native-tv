/** Runs `work` over every item, at most `limit` at a time, results in input order. */
export async function mapPool<T, R>(
  items: readonly T[],
  limit: number,
  work: (item: T) => Promise<R>,
): Promise<R[]> {
  const results: R[] = [];
  let next = 0;
  const worker = async (): Promise<void> => {
    const index = next++;
    const item = items[index];
    if (index >= items.length) return;
    if (item !== undefined) results[index] = await work(item);
    return worker();
  };
  await Promise.all(Array.from({ length: Math.min(limit, items.length) }, worker));
  return results;
}
