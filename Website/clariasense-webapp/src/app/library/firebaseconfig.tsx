// /lib/firebase.ts

import { initializeApp, getApps, FirebaseApp } from 'firebase/app';
import { getDatabase } from 'firebase/database';
import { getFirestore, collection, onSnapshot, query, orderBy } from 'firebase/firestore';
import dotenv from 'dotenv';
import { getFirestore } from 'firebase/firestore';

dotenv.config();

const firebaseConfig = {
  apiKey: process.env.API_KEY,
  authDomain: process.env.AUTH_DOMAIN,
  databaseURL: process.env.DATABASE_URL,
  projectId: "clariasense",
  storageBucket: process.env.STORAGE_BUCKET,
  messagingSenderId: process.env.MESSAGING_SENDER_ID,
  appId: process.env.APP_ID,
  measurementId: process.env.MEASUREMENT_ID,
};


<<<<<<< HEAD
const firebaseApp = initializeApp(firebaseConfig);
const rtdb = getDatabase(firebaseApp);
const firestore = getFirestore(firebaseApp);


export { firebaseApp, rtdb as database, firestore };
=======
const database = getDatabase(firebaseApp);
const firestore = getFirestore(firebaseApp);


export { database, firestore }; 
>>>>>>> 3d0bf2f735fcea9c9f37163c391fad62823d71e3
