'use client';
import Image from 'next/image'


import { useState } from 'react';
import { AnimatePresence, motion } from 'framer-motion';
import Link from 'next/link'; // Import Next.js Link

export default function About() {
    const [menuOpen, setMenuOpen] = useState(false);

    const toggleMenu = () => {
        setMenuOpen(!menuOpen);
    };

    return (
        <div>
            <motion.nav 
                initial={{ height: "auto" }}
                animate={{ height: menuOpen ? "auto" : "auto" }}
                transition={{ duration: 0.5, ease: "easeInOut" }}
                className="bg-white fixed w-full z-20 top-0 start-0 border-b border-gray-200"
            >
                <div className="max-w-screen-xl flex flex-wrap items-center justify-between mx-auto p-4">
                    {/* Logo Here */}
                    <div className=''>
                      <Image
                        src="/images/clariaSenseLogo.png"
                        width={100}
                        height={67}
                        alt="Logo"
                      >
                      </Image>
                    </div>

                    {/* Mobile menu button */}
                    <div className='lg:hidden'>
                      <button onClick={toggleMenu} className="text-4xl" title="Toggle Menu">
                        <svg xmlns="http://www.w3.org/2000/svg" 
                            className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d={menuOpen ? "M6 18L18 6M6 6l12 12" : "M4 6h16M4 12h16M4 18h16"} />
                        </svg>
                      </button>
                    </div>

                    {/* Desktop Menu */}
                    <div className='hidden lg:flex space-x-6'>
                      <Link href="/" className='text-black opacity-100 hover:opacity-70 duration-300'>Home</Link>
                      <Link href="/logs" className='text-black opacity-100 hover:opacity-70 duration-300'>Logs</Link>
                      <Link href="/about" className='text-black opacity-100 hover:opacity-70 duration-300'>About</Link>
                    </div>
                </div>

                {/* Mobile Menu */}
                <AnimatePresence>
                    {menuOpen && (
                      <motion.div
                        initial={{ opacity: 0, y: -20 }}
                        animate={{ opacity: 1, y: 0 }}
                        exit={{ opacity: 0, y: -20 }}
                        transition={{ duration: 0.3 }}
                        className="lg:hidden"
                      >
                        <ul className='space-y-4 mx-5 py-2'>
                          <li><Link href="/" className='text-black opacity-70 hover:opacity-100 duration-300'>Home</Link></li>
                          <li><Link href="/logs" className='text-black opacity-70 hover:opacity-100 duration-300'>Logs</Link></li>
                          <li><Link href="/about" className='text-black opacity-70 hover:opacity-100 duration-300'>About</Link></li>
                        </ul>
                      </motion.div>
                    )}
                </AnimatePresence>
            </motion.nav>

            <main className="text-center mt-40">
                <h1 className="text-4xl">TEST</h1>
                <h2 className="mt-10 text-xl">Responsive Navigation Bar Example</h2>
            </main>
        </div>
    );
}
