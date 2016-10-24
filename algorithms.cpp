/* --                                                         //  {{{1

  File        : algorithms.cpp
  Maintainer  : Felix C. Stegerman <flx@obfusk.net>
  Date        : 2016-10-24

  Copyright   : Copyright (C) 2016  Felix C. Stegerman
  Version     : v0.0.1
  License     : GPLv3+

-- */                                                         //  }}}1

/* ... TODO ... */

#include <functional>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include <deque>
#include <iostream>
#include <vector>

using std::begin;
using std::end;

template <class T>
using unq = typename std::remove_const<
            typename std::remove_reference<T>::type>::type;

template <class T>
class optional                                                //  {{{1
{
public:
  using  stored_t = unq<T>;
  struct nullopt_t {};
  static nullopt_t nullopt;
  class bad_optional_access : std::logic_error
  {
  public:
    bad_optional_access() : std::logic_error("no value") {}
  };
private:
  struct _empty_t {};
  union { _empty_t _empty; stored_t _value; };
  bool  _has_value = false;
private:
  template<class... Args>
  void _construct(Args&&... args)
  { ::new(std::addressof(_value)) stored_t(std::forward<Args...>(args...));
    _has_value = true; }
  void _destruct() { _has_value = false; _value.~stored_t(); }
public:
  optional()            : _empty{} {}
  optional(nullopt_t)   : optional() {}
  optional(const T& t)  : _value(t), _has_value(true) {}
  ~optional()             { reset(); }

  optional& operator = (nullopt_t)
  { reset(); return *this; }
  template<class U>
  optional& operator = (U&& value)
  {
    if (_has_value) _value = std::forward<U>(value);
    else            _construct(std::forward<U>(value));
    return *this;
  }

  template<class... Args>
  void emplace(Args&&... args)
  { reset(); _construct(std::forward<Args>(args)...); }
  template<class U, class... Args>
  void emplace(std::initializer_list<U> ilist, Args&&... args)
  { reset(); _construct(ilist, std::forward<Args>(args)...); }
  void reset()
  { if (_has_value) _destruct(); }

  T& value() &
  { if (!_has_value) throw bad_optional_access(); return _value; }
  const T& value() const &
  { if (!_has_value) throw bad_optional_access(); return _value; }
  T&& value() &&
  { if (!_has_value) throw bad_optional_access(); return std::move(_value); }
  const T&& value() const &&
  { if (!_has_value) throw bad_optional_access(); return std::move(_value); }

  T& operator*() &                { return _value; }
  const T& operator*() const &    { return _value; }
  T&& operator*() &&              { return std::move(_value); }
  const T&& operator*() const &&  { return std::move(_value); }

  template<class U>
  T value_or(U&& d) &&
  { return _has_value ? std::move(**this)
                      : static_cast<T>(std::forward<U>(d)); }
  template<class U>
  T value_or(U&& d) const &
  { return _has_value ? **this
                      : static_cast<T>(std::forward<U>(d)); }

  explicit operator bool() const  { return _has_value; }
  bool has_value()         const  { return _has_value; }
};                                                            //  }}}1

/* ... TODO ... */

template <class T, class ItA, class ItB>
class Chain                                                   //  {{{1
{
public:
  class iterator                                              //  {{{2
  {
  private:
    Chain c;
  public:
    iterator(Chain c) : c(c) {}
    bool not_at_end()
    {
      return c.it_a != c.end_a || c.it_b != c.end_b;
    }
    bool operator != (const iterator&)
    {
      return not_at_end();
    }
    void operator++()
    {
      if      (c.it_a != c.end_a) ++c.it_a;
      else if (c.it_b != c.end_b) ++c.it_b;
    }
    unq<T> operator*()
    {
      if      (c.it_a != c.end_a) return *c.it_a;
      else if (c.it_b != c.end_b) return *c.it_b;
      else throw std::out_of_range(
        "Chain::iterator::operator*(): end reached");
    }
  };                                                          //  }}}2
private:
  ItA it_a; const ItA end_a;
  ItB it_b; const ItB end_b;
public:
  Chain(const ItA& begin_a, const ItA& end_a,
        const ItB& begin_b, const ItB& end_b)
    : it_a(begin_a), end_a(end_a), it_b(begin_b), end_b(end_b) {}
  Chain(const Chain&) = default;
//Chain(Chain&& rhs)
//  : it_a(rhs.it_a), end_a(rhs.end_a),
//    it_b(rhs.it_b), end_b(rhs.end_b)
//{
//  std::cout << "*** Chain MOVE ***" << std::endl;           //  TODO
//}
  iterator begin() { return iterator(*this); }
  iterator end()   { return iterator(*this); }
};                                                            //  }}}1

template <class SeqA, class SeqB>
auto chain(SeqA&& seq_a, SeqB&& seq_b)                        //  {{{1
  -> Chain<decltype(*begin(seq_a)), decltype(begin(seq_a)),
                                    decltype(begin(seq_b))>
{
  return Chain<decltype(*begin(seq_a)), decltype(begin(seq_a)),
                                        decltype(begin(seq_b))>
    (begin(seq_a), end(seq_a), begin(seq_b), end(seq_b));
}                                                             //  }}}1

template <class F, class T, class It>
class Filter                                                  //  {{{1
{
public:
  class iterator                                              //  {{{2
  {
  private:
    Filter c; optional<const unq<T>> v; bool peeked;
  private:
    void peek()
    {
      if (!peeked) {
        peeked = true;
        while (c.it != c.end_) {
          v.emplace(*c.it); ++c.it; if (c.f(*v)) break;
        }
        if (!(c.it != c.end_)) v.reset();
      }
    }
  public:
    iterator(Filter c) : c(c), v(), peeked(false) {}
    bool not_at_end()
    {
      peek(); return v.has_value();
    }
    bool operator != (const iterator&)
    {
      return not_at_end();
    }
    void operator++()
    {
      if(not_at_end()) peeked = false;
    }
    unq<T> operator*()
    {
      if (not_at_end()) return *v;
      throw std::out_of_range(
        "Filter::iterator::operator*(): end reached");
    }
  };                                                          //  }}}2
private:
  const F f; It it; const It end_;
public:
  Filter(F f, const It& begin, const It& end_)
    : f(f), it(begin), end_(end_) {}
  Filter(const Filter&) = default;
//Filter(Filter&& rhs)
//  : f(rhs.f), it(rhs.it), end_(rhs.end_)
//{
//  std::cout << "*** Filter MOVE ***" << std::endl;          //  TODO
//}
  iterator begin() { return iterator(*this); }
  iterator end()   { return iterator(*this); }
};                                                            //  }}}1

template <class F, class Seq>
auto filter(F f, Seq&& seq)                                   //  {{{1
  -> Filter<F, decltype(*begin(seq)), decltype(begin(seq))>
{
  return Filter<F, decltype(*begin(seq)), decltype(begin(seq))>
    (f, begin(seq), end(seq));
}                                                             //  }}}1

template <class F, class T, class It>
class Map                                                     //  {{{1
{
public:
  class iterator                                              //  {{{2
  {
  private:
    Map c;
  public:
    iterator(Map c) : c(c) {}
    bool not_at_end()
    {
      return c.it != c.end_;
    }
    bool operator != (const iterator&)
    {
      return not_at_end();
    }
    void operator++()
    {
      if (not_at_end()) ++c.it;
    }
    unq<T> operator*()
    {
      if (not_at_end()) return c.f(*c.it);
      else throw std::out_of_range(
        "Map::iterator::operator*(): end reached");
    }
  };                                                          //  }}}2
private:
  const F f; It it; const It end_;
public:
  Map(F f, const It& begin, const It& end_)
    : f(f), it(begin), end_(end_) {}
  Map(const Map&) = default;
//Map(Map&& rhs)
//  : f(rhs.f), it(rhs.it), end_(rhs.end_)
//{
//  std::cout << "*** Map MOVE ***" << std::endl;             //  TODO
//}
  iterator begin() { return iterator(*this); }
  iterator end()   { return iterator(*this); }
};                                                            //  }}}1

template <class F, class Seq>
auto map(F f, Seq&& seq)                                      //  {{{1
  -> Map<F, decltype(f(*begin(seq))), decltype(begin(seq))>
{
  return Map<F, decltype(f(*begin(seq))), decltype(begin(seq))>
    (f, begin(seq), end(seq));
}                                                             //  }}}1

template <class T, class It>
class Slice                                                   //  {{{1
{
public:
  class iterator                                              //  {{{2
  {
  private:
    Slice c; long n;
  private:
    bool not_done()
    {
      return (c.stop == -1 || n < c.stop) && c.it != c.end_;
    }
    void fwd()
    {
      while (c.start > 0 && not_done()) { ++c.it; ++n; --c.start; }
    }
  public:
    iterator(Slice c) : c(c), n(0) {}
    bool not_at_end()
    {
      fwd(); return not_done();
    }
    bool operator != (const iterator&)
    {
      return not_at_end();
    }
    void operator++()
    {
      fwd();
      for (long i = 0; i < c.step && not_done(); ++i, ++c.it, ++n);
    }
    unq<T> operator*()
    {
      if (not_at_end()) return *c.it;
      else throw std::out_of_range(
        "Slice::iterator::operator*(): end reached");
    }
  };                                                          //  }}}2
private:
  It it; const It end_; size_t start; const long stop, step;
public:
  Slice(const It& begin, const It& end_, const size_t& start,
        const long& stop, const long& step)
    : it(begin), end_(end_), start(start), stop(stop), step(step)
  {
    if (step < 0)
      throw std::invalid_argument("Slice(): step < 0");
    if (stop < -1)
      throw std::invalid_argument("Slice(): stop < -1");
  }
  Slice(const Slice&) = default;
//Slice(Slice&& rhs)
//  : it(rhs.it), end_(rhs.end_),
//    start(rhs.start), stop(rhs.stop), step(rhs.step)
//{
//  std::cout << "*** Slice MOVE ***" << std::endl;           //  TODO
//}
  iterator begin() { return iterator(*this); }
  iterator end()   { return iterator(*this); }
};                                                            //  }}}1

// ???
template <class Seq>
auto slice(Seq&& seq, const size_t& start,                    //  {{{1
           const long& stop, const long& step = 1)
  -> Slice<unq<decltype(*begin(seq))>, decltype(begin(seq))>
{
  return Slice<unq<decltype(*begin(seq))>, decltype(begin(seq))>
    (begin(seq), end(seq), start, stop, step);
}                                                             //  }}}1

// ???
template <class Seq>
auto slice(Seq&& seq, const long& stop)                       //  {{{1
  -> Slice<unq<decltype(*begin(seq))>, decltype(begin(seq))>
{
  return Slice<unq<decltype(*begin(seq))>, decltype(begin(seq))>
    (begin(seq), end(seq), 0, stop, 1);
}                                                             //  }}}1

template <class F, class T, class It>
class TakeWhile                                               //  {{{1
{
public:
  class iterator                                              //  {{{2
  {
  private:
    TakeWhile c; optional<const unq<T>> v; bool peeked;
  private:
    void peek()
    {
      if (!peeked) {
        peeked = true;
        if (c.it != c.end_) {
          v.emplace(*c.it); ++c.it; if (c.f(*v)) return;
        }
        v.reset();
      }
    }
  public:
    iterator(TakeWhile c) : c(c), v(), peeked(false) {}
    bool not_at_end()
    {
      peek(); return v.has_value();
    }
    bool operator != (const iterator&)
    {
      return not_at_end();
    }
    void operator++()
    {
      if (not_at_end()) peeked = false;
    }
    unq<T> operator*()
    {
      if (not_at_end()) return *v;
      throw std::out_of_range(
        "TakeWhile::iterator::operator*(): end reached");
    }
  };                                                          //  }}}2
private:
  F f; It it; const It end_;
public:
  TakeWhile(F f, const It& begin, const It& end_)
    : f(f), it(begin), end_(end_) {}
  TakeWhile(const TakeWhile&) = default;
//TakeWhile(TakeWhile&& rhs)
//  : f(rhs.f), it(rhs.it), end_(rhs.end_)
//{
//  std::cout << "*** TakeWhile MOVE ***" << std::endl;       //  TODO
//}
  iterator begin() { return iterator(*this); }
  iterator end()   { return iterator(*this); }
};                                                            //  }}}1

template <class F, class Seq>
auto take_while(F f, Seq&& seq)                               //  {{{1
  -> TakeWhile<F, decltype(*begin(seq)), decltype(begin(seq))>
{
  return TakeWhile<F, decltype(*begin(seq)), decltype(begin(seq))>
    (f, begin(seq), end(seq));
}                                                             //  }}}1

template <class S, class T, class ItA, class ItB>
class Zip                                                     //  {{{1
{
public:
  class iterator                                              //  {{{2
  {
  private:
    Zip c;
  public:
    iterator(Zip c) : c(c) {}
    bool not_at_end()
    {
      return c.it_a != c.end_a && c.it_b != c.end_b;
    }
    bool operator != (const iterator&)
    {
      return not_at_end();
    }
    void operator++()
    {
      if (not_at_end()) { ++c.it_a; ++c.it_b; }
    }
    std::tuple<unq<S>, unq<T>> operator*()
    {
      if (not_at_end()) return std::make_tuple(*c.it_a, *c.it_b);
      else throw std::out_of_range(
        "Zip::iterator::operator*(): end reached");
    }
  };                                                          //  }}}2
private:
  ItA it_a; const ItA end_a;
  ItB it_b; const ItB end_b;
public:
  Zip(const ItA& begin_a, const ItA& end_a,
      const ItB& begin_b, const ItB& end_b)
    : it_a(begin_a), end_a(end_a), it_b(begin_b), end_b(end_b) {}
  Zip(const Zip&) = default;
//Zip(Zip&& rhs)
//  : it_a(rhs.it_a), end_a(rhs.end_a),
//    it_b(rhs.it_b), end_b(rhs.end_b)
//{
//  std::cout << "*** Zip MOVE ***" << std::endl;             //  TODO
//}
  iterator begin() { return iterator(*this); }
  iterator end()   { return iterator(*this); }
};                                                            //  }}}1

template <class SeqA, class SeqB>
auto zip(SeqA&& seq_a, SeqB&& seq_b)                          //  {{{1
  -> Zip<decltype(*begin(seq_a)), decltype(*begin(seq_b)),
         decltype( begin(seq_a)), decltype( begin(seq_b))>
{
  return Zip<decltype(*begin(seq_a)), decltype(*begin(seq_b)),
             decltype( begin(seq_a)), decltype( begin(seq_b))>
    (begin(seq_a), end(seq_a), begin(seq_b), end(seq_b));
}                                                             //  }}}1

class IndexError : public std::out_of_range
{
public:
  IndexError() : std::out_of_range("invalid index") {}
};

class StopIteration : public std::out_of_range
{
public:
  StopIteration() : std::out_of_range("end reached") {}
};

class _generator
{
protected:
  int _line;
public:
  _generator() : _line(0) {}
};

#define $generator(NAME)  struct NAME : public _generator
#define $gbegin(TYPE)     TYPE operator()() {                     \
                            switch(_line) {                       \
                              case 0:;
#define $yield(VALUE)         do {                                \
                                _line = __LINE__; return (VALUE); \
                                case __LINE__:;                   \
                              } while (0);
#define $gend               }                                     \
                            _line = 0; throw StopIteration();     \
                          }

template <class T>
class Generator                                               //  {{{1
{
public:
  class iterator                                              //  {{{2
  {
  private:
    Generator g; optional<const unq<T>> v; bool peeked;
  private:
    void peek()
    {
      if (!peeked) {
        peeked = true;
        try                           { v.emplace(g.next()); }
        catch (const StopIteration &) { v.reset(); }
      }
    }
  public:
    iterator(Generator g) : g(g), v(), peeked(false) {}
    bool not_at_end()
    {
      peek(); return v.has_value();
    }
    bool operator != (const iterator&)
    {
      return not_at_end();
    }
    void operator++()
    {
      if (not_at_end()) peeked = false;
    }
    unq<T> operator*()
    {
      if (not_at_end()) return *v;
      throw std::out_of_range(
        "Generator::iterator::operator*(): end reached");
    }
  };                                                          //  }}}2
private:
  std::function<unq<T>()> next;
public:
  template <class F>
  Generator(F next) : next(next) {}
  Generator(const Generator&) = default;
  Generator(Generator&& rhs) : next(rhs.next)
  {
    std::cout << "*** Generator MOVE ***" << std::endl;       //  TODO
  }
  iterator begin() { return iterator(*this); }
  iterator end()   { return iterator(*this); }
};                                                            //  }}}1

template <class T, class F>
auto generator(F next) -> Generator<decltype(next())>
{ return Generator<decltype(next())>(next); }

// TODO: thread-safe w/ locking
template <class T, class It>
class LList                                                   //  {{{1
{
public:
  enum class _next { free, bound, late, done };
  class iterator                                              //  {{{2
  {
  private:
    LList& l; size_t n; const unq<T>* v;
  private:
    void get()
    {
      try { v = &l[n]; } catch (const IndexError &) { v = nullptr; }
    }
  public:
    iterator(LList& l) : l(l), n(0), v(nullptr) {}
    bool not_at_end()
    {
      get(); return v != nullptr;
    }
    bool operator != (const iterator&)
    {
      return not_at_end();
    }
    void operator++()
    {
      ++n;
    }
    const unq<T>& operator*()
    {
      if (not_at_end()) return *v;
      throw std::out_of_range(
        "LList::iterator::operator*(): end reached");
    }
  };                                                          //  }}}2
private:
  std::deque<unq<T>> data; It it; const It end_;
  std::function<unq<T>()> next; _next has_next;
public:
  LList(const It& begin, const It& end_)
    : it(begin), end_(end_), next(static_cast<unq<T>(*)()>(nullptr)),
      has_next(_next::free) {}
  LList(const LList& rhs) = delete;
  LList(LList&& rhs)
    : data(std::move(rhs.data)), it(rhs.it), end_(rhs.end_),
      next(rhs.next), has_next(rhs.has_next)
  {
    std::cout << "*** LList MOVE ***" << std::endl;           //  TODO
  }
  iterator begin() { return iterator(*this); }
  iterator end()   { return iterator(*this); }
  template <class F>
  LList& append(F next_)
  {
    if (has_next != _next::free) {
      if (has_next == _next::late)
        throw std::invalid_argument("LList::append(): late");
      else
        throw std::invalid_argument("LList::append(): again");
    }
    next = next_; has_next = _next::bound;
    return *this;
  }
  const unq<T>& operator[](size_t i)
  {
    for (; i >= data.size() && it != end_; ++it) data.push_back(*it);
    if (i >= data.size()) {
      if (has_next == _next::free) has_next = _next::late;
    }
    while (i >= data.size() && has_next == _next::bound) {
      unq<T> v;
      try { v = next(); }
      catch (const StopIteration&) { has_next = _next::done; break; }
      data.push_back(v);
    }
    if (i >= data.size()) throw IndexError();
    return data[i];
  }
  Slice<unq<T>, iterator>
  operator()(size_t start, long stop, long step = 1)
  {
    return slice(*this, start, stop, step);
  }
  Slice<unq<T>, iterator>
  operator()(long stop)
  {
    return slice(*this, stop);
  }
};                                                            //  }}}1

template <class Seq>
auto llist(Seq&& seq)                                         //  {{{1
  -> LList<decltype(*begin(seq)), decltype(begin(seq))>
{
  return LList<decltype(*begin(seq)), decltype(begin(seq))>
    (begin(seq), end(seq));
}                                                             //  }}}1

template <class T>
auto llist() -> LList<T, char*>
{ return LList<T, char*> (nullptr, nullptr); }

// erastothenes
// ...

/* ... TODO ... */

int main()                                                    //  {{{1
{
  using namespace std;

  const vector<int> a = {1 , 2 ,  3,  4,  5};
  const deque<int>  b = {6 , 7 ,  8,  9, 10};
  const vector<int> c = {11, 12, 13, 14, 15};

  {
    cout <<  "chain(a, b)" << endl;
    auto xs = chain(a, b);
    for (auto x : xs) cout << x << " ";
    cout << endl;
  }

  {
    cout <<  "chain(a, chain(b, c))" << endl;
    auto xs = chain(a, chain(b, c));
    for (auto x : xs) cout << x << " ";
    cout << endl;
  }

  {
    cout <<  "filter({ x % 2 == 0 }, chain(a, b))" << endl;
    auto xs = filter([](int x){ return x % 2 == 0; }, chain(a, b));
    for (auto x : xs) cout << x << " ";
    cout << endl;
  }

  {
    cout <<  "map({ x*x }, chain(a, b))" << endl;
    auto xs = map([](int x){ return x*x; }, chain(a, b));
    for (auto x : xs) cout << x << " ";
    cout << endl;
  }

  {
    cout <<  "slice(chain(a, b), 0, -1, 2)" << endl;
    auto xs = slice(chain(a, b), 0, -1, 2);
    for (auto x : xs) cout << x << " ";
    cout << endl;
  }

  {
    cout <<  "slice(chain(a, b), 3, 6, 2)" << endl;
    auto xs = slice(chain(a, b), 3, 6, 2);
    for (auto x : xs) cout << x << " ";
    cout << endl;
  }

  {
    cout <<  "take_while({ x < 7 }, chain(a, b))" << endl;
    auto xs = take_while([](int x){ return x < 7; }, chain(a, b));
    for (auto x : xs) cout << x << " ";
    cout << endl;
  }

  {
    cout <<  "zip(a, b)" << endl;
    auto xs = zip(a, b);
    for (auto x : xs) cout << get<0>(x) << "," << get<1>(x) << " ";
    cout << endl;
  }

  {
    cout <<  "zip(map({ x*x }, chain(a, b)), c)" << endl;
    auto xs = zip(map([](int x){ return x*x; }, chain(a, b)), c);
    for (auto x : xs) cout << get<0>(x) << "," << get<1>(x) << " ";
    cout << endl;
  }

  {
    cout << "generator(10, i > 0, i--)" << endl;
    int i = 10;
    auto g = generator<int>([&i](){ if (i == 0) throw StopIteration();
                                    return i--; });
    for (auto x : g) cout << x << " ";
    cout << endl;
  }

  {
    cout << "$generator(37; 1, i < 20, i+=2; 42)" << endl;
    $generator(gen) {
      int i = 0;
      $gbegin(int)
        $yield(37)
        for (i = 1; i < 20; i+=2) $yield(i)
        $yield(42)
      $gend
    } g;
    for (auto x : generator<int>(g)) cout << x << " ";
    cout << endl;
  }

  {
    cout <<  "llist(map({ x*x }, chain(a, b)))" << endl;
    auto xs = llist(map([](int x){ return x*x; }, chain(a, b)));
    cout << "xs[3] = " << xs[3] << ", xs[6] = " << xs[6] << endl;
    for (auto x : xs) cout << x << " ";
    cout << endl;
  }

  {
    cout <<  "llist(1).append({ x++ }) break at > 10" << endl;
    auto xs = llist<int>(); auto x = 1;
    xs.append([&x](){ return x++; });
    for (auto x : xs) {
      if (x > 10) break;
      cout << x << " ";
    }
    cout << endl;
  }

  {
    cout <<  "llist(1).append({ x++ }) stop at > 10" << endl;
    auto xs = llist<int>(); auto x = 1;
    xs.append([&x](){ auto y = x++; if(y > 10) throw StopIteration();
                      return y; });
    for (auto x : xs) {
      cout << x << " ";
    }
    cout << endl;
  }

  const vector<int> init = {0,1}; int i = 0;
  auto fibs = llist(init);
  fibs.append([&fibs,&i](){ ++i; return fibs[i-1] + fibs[i]; });

  {
    cout << "fibs" << endl;
    for (auto x : fibs(10)) cout << x << " ";
    cout << endl;
  }

  {
    cout <<  "llist(map({ x*x }, fibs))" << endl;
    auto xs = llist(map([](int x){ return x*x; }, fibs))(10);
    for (auto x : xs) cout << x << " ";
    cout << endl;
  }

  {
    cout << "fibs2" << endl;
    auto fibs2 = llist(fibs);
    for (auto x : fibs2(15)) cout << x << " ";
    cout << endl;
  }

  {
    cout << "sliced fibs" << endl;
    for (auto x : fibs(0, 17, 2)) cout << x << " ";
    cout << endl;
  }

  {
    cout << "fibs -> map -> take_while -> filter -> zip w/ chain" << endl;
    auto xs =
      llist(
        zip(chain(a, chain(b, c)),
          filter([](int x) { return x % 2 == 1 || x % 3 == 1; },
            take_while([](int x) { return x < 10000; },
              map([](int x){ return x*x; }, fibs)))));
    for (auto x : xs(10)) cout << get<0>(x) << "," << get<1>(x) << " ";
    cout << endl;
    cout << "xs[5]<0> = " << get<0>(xs[5]) <<
          ", xs[7]<0> = " << get<1>(xs[7]) << endl;
  }

  /* ... TODO ... */

  return 0;
}                                                             //  }}}1

// vim: set tw=70 sw=2 sts=2 et fdm=marker :
